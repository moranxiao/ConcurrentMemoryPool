#pragma once

#include "Comm.h"



template<class T>
class ObjectPool {
	inline static void* SystemAlloc(size_t size)
	{
		assert(size > 0);
#ifdef _WIN32
		void* ptr = VirtualAlloc(0, kpages << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
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
		if (_freeList != nullptr)
		{
			void* ptr = _freeList;
			_freeList = NextObj(_freeList);
		}
		else		
		{
			if (_remainSize < sizeof(T))
			{
				size_t allocSize = max(sizeof(T), );
				_mem = (char*)SystemAlloc(allocSize);
				_remainSize = allocSize;
			}
			size_t size = max(sizeof(T), sizeof(void*));
			remainSize -= size;
			ptr = _mem;
			_mem += size;
			
		}
		return ptr;
	}
	void Free(void* ptr)
	{
		NextObj(ptr) = _freeList;
		_freeList = ptr;
	}
private:
	char* _mem = nullptr;
	size_t _remainSize = 0;
	void* _freeList = nullptr;
};