#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;    //CentralCache的饿汉对象

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
    // 获取到size对应哪一个SpanList
    size_t index = SizeClass::Index(size);

    _spanLists[index]._mtx.lock();

    // 获取一个管理空间不为空的span
    Span* span = GetOneSpan(_spanLists[index], size);
    assert(span);   
    assert(span->_freeList);

    // 起初都指向_freeList，让end不断往后走
    start = end = span->_freeList;
    size_t actualNum = 1;  //函数实际的返回值,保证最少返回一块

    // 在end的next不为空的前提下，让end走batchNum - 1步
    size_t i = 0;
    while(i < batchNum - 1 && ObjNext(end) != nullptr)
    {
        end = ObjNext(end);
        ++actualNum;  // 记录end走过了多少步
        ++i; 
    }

    //将这一部分空间从Span中删掉
    span->_freeList = ObjNext(end);
    span->use_count += actualNum; // 给tc分了多少就给useCount加多少
    //不要和原先Span的_freeList中的块相连
    ObjNext(end) = nullptr;

    _spanLists[index]._mtx.unlock();    //习惯性写上解锁

    return actualNum;
}

// 获取一个管理空间不为空的span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{       
    // 先在cc中找一下有没有管理空间非空的span
    Span* it = list.Begin();
    while(it != list.End())
    {
        if(it->_freeList != nullptr)  //找到了
            return it;
        else
            it = it->_next;
    }

	// 解掉桶锁，让其他向该cc桶进行操作的线程能拿到锁
    list._mtx.unlock();

    // 没有再向pc中申请新的span
    // 将size转换为匹配的页数
    size_t k = SizeClass::NumMovePage(size);
    // 对pc的操作需要上锁
    PageCache::GetInstance()->_pageMtx.lock();
    // 调用NewSpan获取一个全新的span
    Span* span = PageCache::GetInstance()->NewSpan(k);
    PageCache::GetInstance()->_pageMtx.unlock();

    /* 这里要强转一下，因为_pageID是PageID类型(size_t)的，不能直接赋值给指针*/
    char* start = (char*)(span->_pageID << PAGE_SHIFT);  //_pageID乘上PAGE_SHIFT即这个空间首页的地址
    char* end = (char*)(start + (span->_n << PAGE_SHIFT));  //span的末地址

    // 切分span的空间
    /* 管理的空间放到span->_freeList中 上面的地址是真实物理地址经过处理，
    真实地址转化为无符号整数，除以PAGE_SHIFT得到页数，经过处理这个页数一定是对齐的整数页*/
    span->_freeList = start;  

    void* tail = start;   
    start += size;

    //链接各个块
    while(start < end)
    {
        ObjNext(tail) = start;
        start += size;
        tail = ObjNext(tail);
    }
    ObjNext(tail) = nullptr;   // 记得要把最后一位置空

    // 切好span以后，需要把span挂到cc对应下标的桶里面去	
    list._mtx.lock();  // span挂上之前加锁
    list.PushFront(span);

    return span;
}

// 将tc还回来的多块空间放到span中,size是对齐过的
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{

}