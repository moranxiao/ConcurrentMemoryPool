#pragma once

#include "Comm.h"
#include "ObjectPool.h"
#include "PageMap.h"

class PageCache {
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}
	Span* NewSpan(size_t kpages);

	Span* MapObjToSpan(void* obj);

	void ReleaseSpan(Span* span);

	std::mutex* Mutex()
	{
		return &_mtx;
	}
private:
	SpanList _pageLists[KPAGE];

	////pageId索引得到Span
	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	
	//用radix优化unordered_map，unordered_map需要加锁，而radix不需要加锁大大提高的性能
	TCMalloc_PageMap2<32 - PAGE_SHIFT> _idSpanMap;

	//对象池，用于申请和释放Span对象
	ObjectPool<Span> _spanPool;
	std::mutex _mtx;

private:
	PageCache()
	{}
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;
	static PageCache _sInst;
};