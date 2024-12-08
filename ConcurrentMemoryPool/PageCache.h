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

	////pageId�����õ�Span
	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	
	//��radix�Ż�unordered_map��unordered_map��Ҫ��������radix����Ҫ���������ߵ�����
	TCMalloc_PageMap2<32 - PAGE_SHIFT> _idSpanMap;

	//����أ�����������ͷ�Span����
	ObjectPool<Span> _spanPool;
	std::mutex _mtx;

private:
	PageCache()
	{}
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;
	static PageCache _sInst;
};