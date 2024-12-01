#include "PageCache.h"
#include "CentralCache.h"

PageCache PageCache::_sInst;

Span* PageCache::NewSpan(size_t kpages)
{
	assert(kpages >= 1 && kpages <= 128);
	//����һ��Ͱ��û��Span
	if (!_pageLists[kpages].Empty())
	{
		return _pageLists[kpages].PopFront();
	}
	
	for (size_t i = kpages + 1; i < KPAGE; i++)
	{
		//�����kpages���Ͱ����span��ֱ�ӷָ�span
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
	//�����kpages���Ͱ��û��span������ϵͳ����

	//��ϵͳ������ڴ�������ַ��ֱ��ӳ��Ϊҳ��
	//ֻҪ��֤PAGE��ϵͳҳ����һ����С����

	void* ptr = SystemAlloc(KPAGE-1);
	Span* span = new Span;
	span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	span->_n = KPAGE-1;
	_pageLists[span->_n].PushFront(span);
	return NewSpan(kpages);
}
