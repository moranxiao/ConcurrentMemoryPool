#pragma once

#include "Comm.h"

class ThreadCache {
public:
	void* Allocate(size_t size);

	//void Deallocate();

	void* FetchFromCentralCache(size_t index, size_t size);

	void FreeObj(void* ptr,size_t size);

	void ListTooLong(FreeList& list, size_t size);

private:
	FreeList _freeLists[FREELISTS_NUM];
		
};

static __declspec(thread) ThreadCache* pTLSThreadCache = nullptr;

