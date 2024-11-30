#pragma once
#include "ThreadCache.h"

namespace concurrent_mem_pool {

	static void* ConcurrentAllocate(size_t size)
	{
		if (pTLSThreadCache == nullptr)
		{
			pTLSThreadCache = new ThreadCache;
		}
		return pTLSThreadCache->Allocate(size);
	}
}