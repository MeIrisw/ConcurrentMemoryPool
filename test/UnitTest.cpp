#include "ConcurrentAlloc.h"

//线程1
void Alloc1()
{
    for(int i = 0;i < 5; ++i)
    {
        ConcurrentAlloc(6);
    }
}

//线程2
void Alloc2()
{
    for(int i = 0;i < 5; ++i)
    {
        ConcurrentAlloc(7);
    }
}


void AllocTest()
{
    
}

int main(int argc,const char* argv[])
{
    AllocTest();
    return 0;
}