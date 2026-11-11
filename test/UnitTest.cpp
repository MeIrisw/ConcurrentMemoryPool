#include "ConcurrentAlloc.h"
#include <thread>  //使用C++的std::thread

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
    std::thread t1(Alloc1);
    std::thread t2(Alloc2);

    t1.join();
    t2.join();
}

void ConcurrentAllocTest1()
{
	void* ptr1 = ConcurrentAlloc(5);
	void* ptr2 = ConcurrentAlloc(8);
	void* ptr3 = ConcurrentAlloc(4);
	void* ptr4 = ConcurrentAlloc(6);
	void* ptr5 = ConcurrentAlloc(3);

	std::cout << ptr1 << std::endl;
	std::cout << ptr2 << std::endl;
	std::cout << ptr3 << std::endl;
	std::cout << ptr4 << std::endl;
	std::cout << ptr5 << std::endl;
}

void ConcurrentAllocTest2()
{
	for (int i = 0; i < 1024; ++i)
	{
		void* ptr = ConcurrentAlloc(5);
		std::cout << ptr << std::endl;
	}

	void* ptr = ConcurrentAlloc(3);
	std::cout << "-----" << ptr << std::endl;
}

int main(int argc,const char* argv[])
{
    ConcurrentAllocTest1();
    return 0;
}