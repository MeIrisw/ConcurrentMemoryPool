#include "ThreadCache.h"

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

    return nullptr;
}