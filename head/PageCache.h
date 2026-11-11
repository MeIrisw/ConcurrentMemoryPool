#ifndef PAGE_CACHE_H
#define PAGE_CACHE_H

#include "Common.h"

class PageCache
{
public:
    // 饿汉单例
    static PageCache* GetInstance()
    {
        return &_sInst;
    }

    // pc从_spanLists中拿出来一个k页的span
    Span* NewSpan(size_t k);

private:
    SpanList _spanLists[PAGE_NUM];    // pc中的哈希表

public:
    std::mutex _pageMtx;        // pc整体的锁

private: // 单例，构造私有，拷构，赋构去掉
    PageCache()
    {}

    PageCache(const PageCache& pc) = delete;
    PageCache& operator = (const PageCache& pc) = delete;

    static PageCache _sInst;    //饿汉模式单例对象

};



#endif