#include "PageCache.h"

PageCache PageCache::_sInst;   //单例对象

// pc从_spanLists中拿出来一个k页的span
Span* PageCache::NewSpan(size_t k)
{
    assert(k > 0 && k < PAGE_NUM);

    // <1> k号桶中有span
    if(!_spanLists[k].Empty())
    {
        return _spanLists[k].PopFront();
    }
   
    // <2> k号桶中没有span，但后面的桶中有span
    for(int i = k + 1; i < PAGE_NUM; ++i)
    {
        // i号桶中有span
        if(!_spanLists[i].Empty())
        {
            Span* nSpan = _spanLists[i].PopFront();

            // 将这个Span切分为一个k页和一个n-k页的span
            // Span的空间是需要新建的，因为划分后需要两个span来管理页，上面只有一个span和页空间
            Span* kSpan = new Span;

            // 分一个k页的span
            kSpan->_pageID = nSpan->_pageID;
            kSpan->_n = k;

            // 和一个n-k页的span
            nSpan->_pageID += k;
            nSpan->_n -= k;

			// n - k页的放回对应哈希桶中
            _spanLists[nSpan->_n].PushFront(nSpan);
            return kSpan;
        }
    }
    
    // <3> k号桶和后面的桶中都没有span
    // 直接向系统申请128页的span
    

}