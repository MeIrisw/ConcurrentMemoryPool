#ifndef CENTRAL_CACHE_H
#define CENTRAL_CACHE_H

#include "Common.h"

class CentralCache
{
public:
    // 单例接口
    static CentralCache* GetInstance()
    {
        return &_sInst;
    }

    // cc向tc提供空间的接口
    // start和end表示cc提供空间的首尾，输出型参数；n表示tc要的数量，size表示单块空间大小，返回值是cc实际提供的空间块数（可能不够，有多少给多少）
    size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);

    // 获取一个管理空间不为空的span
    Span* GetOneSpan(SpanList& list, size_t size);

private:
    // 单例，去掉构造、拷构和拷赋
    CentralCache()
    {}

    CentralCache(const CentralCache& copy) = delete;
    CentralCache& operator =(const CentralCache& copy) = delete;
    
private:
    SpanList _spanLists[FREE_LIST_NUM];    // cc中的哈希桶数量和tc中一致，每个块大小可以一一对应
    static CentralCache _sInst;      // 饿汉模式创建一个CentralCache
}




#endif