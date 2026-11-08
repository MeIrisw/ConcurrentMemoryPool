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


class SizeClass
{
public:
    static size_t RoundUp(size_t size)   //计算对齐字节数
    {
        if(size <= 128)
        { //[1,128] 8B
            return _RoundUp(size, 8);
        }
        else if(size <= 1024)
        { //[128+1,1024] 16B
            return _RoundUp(size, 16);
        }
        else if(size <=  8 * 1024)
        { //[1024+1,8*1024] 128B
            return _RoundUp(size, 128);
        }
        else if(size <= 64 * 1024)
        { //[8*1024+1，64*1024] 1024B
            return _RoundUp(size, 1024);
        }
        else if(size <= 256 * 1024)
        { //[64*1024+1,256*1024] 8*1024B
            return _RoundUp(size, 8 * 1024);
        }
        else
        { //不可能的情况，tc申请空间不会超过256KB
            assert(false);
            return -1;
        }
    }

    //计算每个分区对齐后的字节数，alignNum是size对应分区的对齐字节数
    static size_t _RoundUp(size_t size, size_t alignNum)
    {
        size_t res = 0;
        if(size % alignNum != 0)
        { //有余数，要多给一个对齐，比如size = 3,这里就是(3 / 8 + 1) * 8 = 8
            res = (size / alignNum + 1) * alignNum;
        }
        else
        { //没有余数，本身就是对齐的
            res = size;
        }
        return res;
    }

    // 求size对应在哈希表中的下标,align_shift是对齐数的二进制位数，比如size是3对齐数为8，align_shift就是3(2^3=8)
    // 这里算的是size所在当前区域的第几个下标，下面Index函数还要加上前面区域的哈希桶的个数
    static inline size_t _Index(size_t size, size_t align_shift)
    {
        return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
    }

    //计算映射的哪一个哈希桶
    static inline size_t Index(size_t size)
    {
        assert(size <= MAX_BYTES);

        //每个区间的链数
        static int group_array[4] = {16,56,56,56};
        if(size <= 128)
        { //[1,128] 8B
            return _Index(size, 3);
        }
        else if(size <= 1024)
        { //[128+1,1024] 16B
            return _Index(size - 128, 4) + group_array[0];
        }
        else if(size <=  8 * 1024)
        { //[1024+1,8*1024] 128B
            return _Index(size - 1024, 7) + group_array[1] + group_array[0];
        }
        else if(size <= 64 * 1024)
        { //[8*1024+1，64*1024] 1024B
            return _Index(size - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
        }
        else if(size <= 256 * 1024)
        { //[64*1024+1,256*1024] 8*1024B
            return _Index(size - 64 * 1024, 13) + group_array[3] + group_array[2] + group_array[1] + group_array[0];
        }
        else
        { 
            assert(false);
        }
        return -1;
    }
};

#endif