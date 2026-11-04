#ifndef THREAD_CACHE_H
#define THREAD_CACHE_H

#include "Common.h"

class ThreadCache
{
public:
    void* Allocate(size_t size);   //线程申请size大小的空间

    void Deallocate(void* obj, size_t size);     //回收线程中大小为size的obj空间
};

#endif