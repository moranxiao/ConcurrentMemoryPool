#include <iostream>
#include <thread>

#include "ConcurrentAlloc.h"


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

//void TestConcurrentFree1()
//{
//	void* p1 = ConcurrentAllocate(1);
//	void* p2 = ConcurrentAllocate(8);
//	void* p3 = ConcurrentAllocate(7);
//	void* p4 = ConcurrentAllocate(5);
//	void* p5 = ConcurrentAllocate(4);
//	void* p6 = ConcurrentAllocate(2);
//	void* p7 = ConcurrentAllocate(8);
//
//	ConcurrentFree(p1, 1);
//	ConcurrentFree(p2, 8);
//	ConcurrentFree(p3, 8);
//	ConcurrentFree(p4, 8);
//	ConcurrentFree(p5, 8);
//	ConcurrentFree(p6, 8);
//	ConcurrentFree(p7, 8);
//
//}

void TestConcurrentFree2()
{
	void* p1 = ConcurrentAllocate(1);
	void* p2 = ConcurrentAllocate(8);
	void* p3 = ConcurrentAllocate(7);
	void* p4 = ConcurrentAllocate(5);
	void* p5 = ConcurrentAllocate(4);
	void* p6 = ConcurrentAllocate(2);
	void* p7 = ConcurrentAllocate(8);

	ConcurrentFree(p1);
	ConcurrentFree(p2);
	ConcurrentFree(p3);
	ConcurrentFree(p4);
	ConcurrentFree(p5);
	ConcurrentFree(p6);
	ConcurrentFree(p7);

}

void TestBigAlloc()
{
	void* p1 = ConcurrentAllocate(MAX_BYTES + 1);
	void* p2 = ConcurrentAllocate(KPAGE*(1 << PAGE_SHIFT) + 1);
	ConcurrentFree(p1);
	ConcurrentFree(p2);
}



//int main()
//{
//	//TLSTest();
//	//TestConcurrentAlloc1();
//	//TestConcurrentAlloc2();
//	//TestConcurrentFree2();
//	//TestBigAlloc();
//	//TestObjPool();
//
//	return 0;
//}