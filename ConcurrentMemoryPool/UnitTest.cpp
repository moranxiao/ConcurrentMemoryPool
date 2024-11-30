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

int main()
{
	TLSTest();
	return 0;
}