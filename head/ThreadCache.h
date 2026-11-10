#ifndef THREAD_CACHE_H
#define THREAD_CACHE_H

#include "Common.h"

class ThreadCache
{
public:
    void* Allocate(size_t size);   //线程申请size大小的空间

    void Deallocate(void* obj, size_t size);     //回收线程中大小为size的obj空间

    //ThreadCache中空间不够时，向CentralCache申请空间的接口
    void* FetchFromCentralCache(size_t index, size_t alignSize);

private:
    FreeList _freeLists[FREE_LIST_NUM];    //哈希表
};

//TLS全局对象的指针，这样每个线程都能有这样一个独立的全局对象 static防止多个.cpp文件包含该文件时会发生链接错误
static __thread ThreadCache* pTLSThreadCache = nullptr;


#endif