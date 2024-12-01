#include <iostream>
#include <thread>

#include "ConcurrentAlloc.h"

using namespace concurrent_mem_pool;
 
void Alloc()
{
	for (int i = 0; i < 5; i++)
	{
		void* ptr = ConcurrentAllocate(7);
	}
}

void TLSTest()
{
	std::thread t1(Alloc);
	t1.join();
	std::thread t2(Alloc);

	t2.join();
}

void TestConcurrentAlloc1()
{
	void* p1 = ConcurrentAllocate(1);
	void* p2 = ConcurrentAllocate(8);
	void* p3 = ConcurrentAllocate(7);
	void* p4 = ConcurrentAllocate(5);
	void* p5 = ConcurrentAllocate(4);
	void* p6 = ConcurrentAllocate(2);
	std::cout << p1 << std::endl;
	std::cout << p2 << std::endl;
	std::cout << p3 << std::endl;
	std::cout << p4 << std::endl;
	std::cout << p5 << std::endl;
	std::cout << p6 << std::endl;

	void* p7 = ConcurrentAllocate(17);
	std::cout << p7 << std::endl;

}

void TestConcurrentAlloc2()
{
	for (int i = 0; i < 1024; i++)
		void* ptr = ConcurrentAllocate(8);
	void* ptr = ConcurrentAllocate(7);
}
int main()
{
	//TLSTest();
	//TestConcurrentAlloc1();
	TestConcurrentAlloc2();
	return 0;
}