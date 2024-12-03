#pragma once

#include "Comm.h"

class CentralCache {
public:
	Span* GetOneSpan(size_t index,size_t size);

	size_t FetchRangeObj(void*& begin, void*& end, size_t batch, size_t size);

	void ReleaseListToSpans(void* begin,size_t size);

	static CentralCache* GetInstance()
	{
		return &_sInst;
	}
private:
	SpanList _spanLists[FREELISTS_NUM];

	static CentralCache _sInst;
private:
	CentralCache()
	{}
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;
};


