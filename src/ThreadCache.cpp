#include "ThreadCache.h"
#include "CentralCache.h"

//线程向tc申请size大小的空间
void* ThreadCache::Allocate(size_t size) 
{
    assert(size <= MAX_BYTES);    

    size_t alignSize = SizeClass::RoundUp(size);    //size对齐后的字节数
    size_t index = SizeClass::Index(size);          //size对应在哈希表中的哪个桶

    if(!_freeLists[index].Empty())
    { //自由链表中不为空,直接从自由链表中获取空间
        return _freeLists[index].Pop();
    }
    else
    { //自由链表为空，得要让ThreadCache向CentralCache申请空间
        return FetchFromCentralCache(index, alignSize);
    }
}

//回收线程中大小为size的obj空间,这里的size保证是一个对齐的size，所以函数里不调RoundUp()对齐的函数
void ThreadCache::Deallocate(void* obj,size_t size)   
{
    assert(obj);
    assert(size <= MAX_BYTES);

    size_t index = SizeClass::Index(size);  //找到size对应的自由链表
    _freeLists[index].Push(obj);          //回收空间
}

//ThreadCache中空间不够时，向CentralCache申请空间的接口
void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize)
{
    // 通过MaxSize和NumMoveSie来控制cc给tc提供多少块alignSize大小的空间
    size_t batchNum = std::min(_freeLists[index].MaxSize(),SizeClass::NumMoveSize(alignSize));
    
    //没有到达上限，下次多给一块，慢开始调节算法的核心
    if(batchNum == _freeLists[index].MaxSize())
    { 
        _freeLists[index].MaxSize()++;
    }
    
    //输出型参数，返回之后的结果就是tc想要的空间
    void* start = nullptr;
    void* end = nullptr;

    //返回值为实际获取到的块数
    size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, alignSize);

    assert(actualNum >= 1);    //FetchRangeObj保证actualNum大于等于1

    //如果actualNum等于1，直接将start返回给线程，线程是一次申请一块的
    if(actualNum == 1)
    { 
        assert(start == end);
        return start;
    }else{
        //如果actulNum大于1，就还要给tc对应位置插入[ObjNext(start), end]的空间
		_freeLists[index].PushRange(ObjNext(start), end);

        return start;  //给线程返回start所指空间
    }
}