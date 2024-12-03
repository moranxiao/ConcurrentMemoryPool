#pragma once
#include "ThreadCache.h"

static void* ConcurrentAllocate(size_t size)
{
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache;
	}
	return pTLSThreadCache->Allocate(size);
}

static void ConcurrentFree(void* ptr, size_t size)
{

}