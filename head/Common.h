#ifndef COMMON_H
#define COMMON_H

#include<iostream>
#include<vector>
#include<pthread.h>
#include<unistd.h>   
#include<cassert>
#include<mutex>

static const size_t FREE_LIST_NUM = 208; //哈希表中自由链表（哈希桶）的个数 每一个tc一个哈希表
static const size_t MAX_BYTES = 256 * 1024;   //ThreadCache单次申请的最大字节数256KB

typedef size_t PageID;

static void*& ObjNext(void* obj)   //obj空间的前4/8个字节
{
    return *(void**)obj;
}

class FreeList
{
public:
    //向自由链表中头插入多块空间
    void PushRange(void* start, void* end)
    {
        ObjNext(end) = _freeList;
        _freeList = start;
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
    }

    void* Pop()         //头删法提供空间
    {
        assert(_freeList); //链表非空

        void* obj = _freeList;
        _freeList = ObjNext(obj);

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
        prev->next = ptr;
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
}

#endif