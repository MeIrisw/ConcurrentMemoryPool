#ifndef COMMON_H
#define COMMON_H

#include<iostream>
#include<vector>

static const size_t FREE_LIST_NUM = 208; //哈希表中自由链表（哈希桶）的个数 每一个tc一个哈希表
static const size_t MAX_BYTES = 256 * 1024;   //ThreadCache单次申请的最大字节数256KB

static void*& ObjNext(void* obj)   //obj空间的前4/8个字节
{
    return *(void**)obj;
}

class FreeList
{
public:
    bool Empty()   //判断哈希桶是否为空
    {
        return _freeList == nullptr;
    }

    void push(void* obj)    //回收的空间头插入链表
    {
        assert(obj);   //空间非空

        objNext(obj) = _freeList;
        _freeList = obj;
    }

    void* pop()         //头删法提供空间
    {
        assert(_freeList); //链表非空

        void* obj = _freeList;
        _freeList = ObjNext(obj);

        return obj;
    }

private:
    void* _freeList = nullptr;    
};

#endif