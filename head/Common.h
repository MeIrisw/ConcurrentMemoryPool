#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <vector>
#include <pthread.h>
#include <unistd.h>   // sysconf  
#include <cassert>
#include <mutex>
#include <cstdint>  // std::uintptr_t：把地址转成整数，以便计算对齐位置
#include <limits>   // 检查申请大小是否溢出
#include <new>      // std::bad_alloc

static const size_t FREE_LIST_NUM = 208; // 哈希表中自由链表（哈希桶）的个数 每一个tc一个哈希表
static const size_t MAX_BYTES = 256 * 1024;   // ThreadCache单次申请的最大字节数256KB
static const size_t PAGE_NUM = 129;    // span的最大管理页数，这里在数组中不用0（因为直接映射，1号桶对应的span管理1页），所以多开了一个
static const size_t PAGE_SHIFT = 13;   // 一页多少位，这里一页设置为8KB,就是13位

typedef size_t PageID;

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>  // mmap、munmap
#endif

/* PAGE_SHIFT 是 13，内存池按 8 KiB 计算页号；而这台 Linux 服务器的系统页是 4 KiB。
 直接返回 mmap 的地址可能破坏页号与地址之间的换算，所以需要额外申请一点空间并对齐到 8 KiB,然后再释放掉。*/
inline static void* SystemAlloc(size_t kpage)
{
    // 内存池的一页是 2^PAGE_SHIFT 字节；目前 PAGE_SHIFT=13，即 8 KiB
    constexpr size_t pageSize = size_t{1} << PAGE_SHIFT;
    void* ptr = nullptr;

#ifdef _WIN32

    // 向 Windows 申请 kpage 个内存池页
    ptr = VirtualAlloc(nullptr, kpage * pageSize,
                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (ptr == nullptr)
        throw std::bad_alloc();
#else
    // 查询 Linux 的系统页大小；这台机器上是 4 KiB
    const long systemPage = sysconf(_SC_PAGESIZE);

    // 排除无效页数、不兼容的系统页大小，以及计算字节数时的溢出
    if (kpage == 0 || systemPage <= 0 ||
        pageSize % static_cast<size_t>(systemPage) != 0 ||
        kpage > (std::numeric_limits<size_t>::max() - pageSize) / pageSize)
        throw std::bad_alloc();

    const size_t bytes = kpage * pageSize;  // 实际需要的字节数

    // 多申请一个内存池页，确保其中能找到 8 KiB 对齐的起点
    const size_t mappedBytes = bytes + pageSize;

    // 申请可读写的匿名私有映射，不关联文件
    void* raw = mmap(nullptr, mappedBytes, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // mmap 失败返回 MAP_FAILED，而不是 nullptr
    if (raw == MAP_FAILED)
        throw std::bad_alloc();

    // prefix：从 mmap 返回地址走到下一个 8 KiB 边界所需的字节数
    // 如果 raw 已经按 8 KiB 对齐，prefix 就是 0
    const size_t prefix =
        (pageSize - (reinterpret_cast<std::uintptr_t>(raw) & (pageSize - 1)))
        & (pageSize - 1);

    char* aligned = static_cast<char*>(raw) + prefix;  // 内存池实际使用的起点
    const size_t suffix = mappedBytes - prefix - bytes; // 所需空间后多出的字节数

    // 释放对齐起点前后多申请的部分，只保留 [aligned, aligned + bytes)
    if (prefix != 0)
        munmap(raw, prefix);
    if (suffix != 0)
        munmap(aligned + bytes, suffix);

    ptr = aligned;
#endif

    return ptr;
}

static void*& ObjNext(void* obj)   //obj空间的前4/8个字节
{
    return *(void**)obj;
}

class FreeList
{
public:
    // 删除掉桶中n个块（头删），并把删除的空间作为输出型参数返回
    void PopRange(void*& start, void*& end, size_t n)
    {
        assert(n <= _size);

        start = end = _freeList;

        for(size_t i = 0; i < n - 1; ++i)
        {
            end = ObjNext(end);
        }

        _freeList = ObjNext(end);
        ObjNext(end) = nullptr;
        _size -= n;
    }
    
    // 获取当前桶中有多少空间
    size_t Size()
    {
        return _size;
    }
    
    //向自由链表中头插入多块空间
    void PushRange(void* start, void* end, size_t size)
    {
        ObjNext(end) = _freeList;
        _freeList = start;

        _size += size;
    }

    bool Empty()   //判断哈希桶是否为空
    {
        return _freeList == nullptr;
    }

    void Push(void* obj)    //回收的空间头插入链表
    {
        assert(obj);   //空间非空

        ObjNext(obj) = _freeList;
        _freeList = obj;

        ++_size;
    }

    void* Pop()         //头删法提供空间
    {
        assert(_freeList); //链表非空

        void* obj = _freeList;
        _freeList = ObjNext(obj);

        --_size;

        return obj;
    }

    //获取_maxSize的接口
    size_t& MaxSize()
    {
        return _maxSize;
    }

private:
    void* _freeList = nullptr;    
    size_t _maxSize = 1;    //当前自由链表申请未达到上限时，能够申请的最大块空间是多少,到了上限这个值就作废 
    size_t _size = 0;    //当前自由链表中有多少块空间
};

//以页为基本单位的结构体
struct Span
{
public:
    PageID _pageID = 0; //页号
    size_t _n = 0;     //当前span一共管理的页的数量

    void* _freeList = nullptr;  //每个span下面挂的小块空间链的头结点
    size_t use_count = 0; //当前span分配出去了多少个块空间

    Span* _prev = nullptr;  //双向链表
    Span* _next = nullptr; 
};

class SpanList
{
public:
    // 删掉并返回第一个span，出队
    Span* PopFront()
    {
        Span* front = _head->_next;
        Erase(front);

        return front;
    }

    // 判空 
    bool Empty()
    {
        return _head == _head->_next;
    }

    //头插
    void PushFront(Span* span)
    {
        Insert(Begin(),span);
    }

    // 头结点
    Span* Begin()
    {
        return _head->_next;
    }

    // 尾结点 就是这个哨兵结点，因为是双向循环链表
    Span* End()
    {
        return _head;
    }

    SpanList()
    { // 构造函数中做哨兵位头结点
        _head = new Span;

        // 因为是双向循环链表，所以一开始都指向_head
        _head->_prev = _head;
        _head->_next = _head;
    }

    //在pos前面插入ptr
    void Insert(Span* pos, Span* ptr)
    { 
        assert(pos);
        assert(ptr);

        Span* prev = pos->_prev;
        prev->_next = ptr;
        ptr->_prev = prev;
        ptr->_next = pos;
        pos->_prev = ptr;
    }

    //删除该结点,pos结点不能用delete删，因为还要回收利用
    void Erase(Span* pos)
    {
        assert(pos); 
        assert(pos != _head);

        Span* prev = pos->_prev;
        Span* next = pos->_next;
        prev->_next = next;
        next->_prev = prev;
    }

private:
    Span* _head; //哨兵位头结点

public:
    std::mutex _mtx;  //每个哈希桶都要有一把锁
};

class SizeClass
{
	// 线程申请size的对齐规则：整体控制在最多10%左右的内碎片浪费
	//	size范围				对齐数				对应哈希桶下标范围
	// [1,128]					8B 对齐      		freelist[0,16)
	// [128+1,1024]			16B 对齐  			freelist[16,72)
	// [1024+1,8*1024]			128B 对齐  			freelist[72,128)
	// [8*1024+1,64*1024]		1024B 对齐    		freelist[128,184)
	// [64*1024+1,256*1024]	8*1024B 对齐  		freelist[184,208)

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

    //向cc申请的上限块数（慢启动上限）
    static size_t NumMoveSize(size_t size)
    {
        assert(size > 0);

        int num = MAX_BYTES / size;  //这里除之后先简单控制一下

        if(num > 512)
        { //如果申请的大小较小，那么块数会很多，容易浪费。所以简单限制一下
            num = 512;
        }

        //如果除了之后特别小，那就改成2
        if(num < 2)
        {
            num = 2;
        }
        //[2,512]即为一次批量移动多少个对象（慢启动）的上限值

        return num;
    }

    // 块页匹配算法  size表示需要的一块的大小
    /* 当cc中没有span为tc提供小块空间时，cc就需要向pc申请一块span，此时需要根据块空间的大小来匹配
	出一个维护页空间较为合适的span，以保证span为size后尽量不浪费或不足够还再频繁申请相同大小的span*/
    static size_t NumMovePage(size_t size)
    {
        //获取tc向cc申请size大小的块时的单次最大申请块数
        size_t num = NumMoveSize(size);

        //单次申请最大空间的大小
        size_t npage = num * size;

        //最大空间的页数
        npage >>= PAGE_SHIFT;
    
        //算出来不到1页给1页
        if(npage == 0)
            npage = 1;

        return npage;
    }
};

#endif