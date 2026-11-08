#include "CentralCache.h"

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
    //不要和原先Span的_freeList中的块相连
    ObjNext(end) = nullptr;

    _spanLists[index]._mtx.unlock();    //习惯性写上解锁

    return actualNum;
}

// 获取一个管理空间不为空的span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{

    return nullptr;
}