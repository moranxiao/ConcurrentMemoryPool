#pragma once

#include "Comm.h"

class ThreadCache {
public:
	void* Allocate(size_t sizeByte);

	void Deallocate();

	void* FetchFromCentralCache(size_t index, size_t size);
private:
	FreeList _freeLists[FREELISTS_NUM];
		
};

static __declspec(thread) ThreadCache* pTLSThreadCache = nullptr;

