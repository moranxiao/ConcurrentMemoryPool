#pragma once

#include "Comm.h"

class PageCache {
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}
	Span* NewSpan(size_t kpages);

	std::mutex* Mutex()
	{
		return &_mtx;
	}
private:
	SpanList _pageLists[KPAGE];
	std::mutex _mtx;
private:
	PageCache()
	{}
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;
	static PageCache _sInst;
};