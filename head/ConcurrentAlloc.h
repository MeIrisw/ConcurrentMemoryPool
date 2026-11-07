#ifndef  CONCURRENT_ALLOC_H
#define CONCURRENT_ALLOC_H

#include "ThreadCache.h"

// 整个项目直接调用的两个接口
// 即tcmalloc，线程调用这个函数来申请空间
void* ConcurrentAlloc(size_t size)
{
    // 因为pTLSThreadCache是TLS的，每个线程都有一个，不会竞争，所以不存在线程安全问题
    if(pTLSThreadCache == nullptr)
    {
        pTLSThreadCache = new ThreadCache;   //此时每个线程就有了一个ThreadChche对象
    }

    return pTLSTHreadCache->Allocate(size);
}

// 线程调用这个函数用来回收空间
void ConcurrentFree(void* ptr)
{

}


#endif