#pragma once
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"

static void* ConcurrentAllocate(size_t size)
{
	if (pTLSThreadCache == nullptr)
	{
		static ObjectPool<ThreadCache> threadPool;
		pTLSThreadCache = threadPool.New();
	}
	assert(pTLSThreadCache);
	if (size > MAX_BYTES)
	{
		size_t alignSize = SizeClass::RoundUp(size);
		size_t kpage = alignSize >> PAGE_SHIFT;
		PageCache::GetInstance()->Mutex()->lock();
		Span* span = PageCache::GetInstance()->NewSpan(kpage);
		PageCache::GetInstance()->Mutex()->unlock();
		span->_objSize = alignSize;
		return (void*)(span->_pageId << PAGE_SHIFT);
	}
	else
	{
		return pTLSThreadCache->Allocate(size);
	}
}

static void ConcurrentFree(void* ptr)
{
	assert(pTLSThreadCache);
	PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
	size_t size = span->_objSize;
	if (size > MAX_BYTES)
	{
		PageCache::GetInstance()->Mutex()->lock();
		PageCache::GetInstance()->ReleaseSpan(span);
		PageCache::GetInstance()->Mutex()->unlock();
	}
	else  pTLSThreadCache->Deallocate(ptr,size);
}