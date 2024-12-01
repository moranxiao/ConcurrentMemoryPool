#include "PageCache.h"
#include "CentralCache.h"

PageCache PageCache::_sInst;

Span* PageCache::NewSpan(size_t kpages)
{
	assert(kpages >= 1 && kpages <= 128);
	//检查第一个桶有没有Span
	if (!_pageLists[kpages].Empty())
	{
		return _pageLists[kpages].PopFront();
	}
	
	for (size_t i = kpages + 1; i < KPAGE; i++)
	{
		//如果比kpages大的桶中有span则直接分割span
		if (!_pageLists[i].Empty())
		{
			Span* span = _pageLists[i].PopFront();
			Span* new_span = new Span;   

			new_span->_pageId = span->_pageId;
			new_span->_n = kpages;

			span->_n -= kpages;
			span->_pageId += kpages;
			_pageLists[span->_n].PushFront(span);

			return new_span;
		}
	}
	//如果比kpages大的桶中没有span，则向系统申请

	//像系统申请的内存根据其地址，直接映射为页号
	//只要保证PAGE和系统页面是一样大小就行

	void* ptr = SystemAlloc(KPAGE-1);
	Span* span = new Span;
	span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	span->_n = KPAGE-1;
	_pageLists[span->_n].PushFront(span);
	return NewSpan(kpages);
}
