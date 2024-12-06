#pragma once

#include "Comm.h"

#ifdef _WIN32
#include <Windows.h>
#else
//linux下头文件
#endif


template<class T>
class ObjectPool {
	static const size_t MAX_BYTES = 128*1024;
	inline static void* SystemAlloc(size_t size)
	{
		assert(size > 0);
#ifdef _WIN32
		void* ptr = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
//linux
#endif
		if (ptr == nullptr) throw std::bad_alloc();
		return ptr;
	}
	inline static void*& NextObj(void* obj)
	{
		return *(void**)obj;
	}
public:
	T* New()
	{
		T* obj = nullptr;
		_mtx.lock();
		if (_freeList != nullptr)
		{
			obj = (T*)_freeList;
			_freeList = NextObj(_freeList);
		}
		else		
		{
			if (_remainSize < sizeof(T))
			{
				size_t allocSize = max(sizeof(T), MAX_BYTES);
				_mem = (char*)SystemAlloc(allocSize);
				_remainSize = allocSize;
			}
			size_t size = max(sizeof(T), sizeof(void*));
			_remainSize -= size;
			obj = (T*)_mem;
			_mem += size;
			
		}
		_mtx.unlock();
		new(obj)T;
		return obj;
	}
	void Delete(T* obj)
	{
		obj->~T();
		_mtx.lock();
		NextObj(obj) = _freeList;
		_freeList = obj;
		_mtx.unlock();
	}

private:
	char* _mem = nullptr;
	size_t _remainSize = 0;
	void* _freeList = nullptr;
	std::mutex _mtx;
};


//static void TestObjPool()
//{
//	ObjectPool<int> objpool;
//	int* p1 = objpool.New();
//	int* p2 = objpool.New();
//	int* p3 = objpool.New();
//	int* p4 = objpool.New();
//	int* p5 = objpool.New();
//	int* p6 = objpool.New();
//	int* p7 = objpool.New();
//
//	objpool.Delete(p1);
//	objpool.Delete(p2);
//	objpool.Delete(p3);
//	objpool.Delete(p4);
//	objpool.Delete(p5);
//	objpool.Delete(p6);
//	objpool.Delete(p7);
//}