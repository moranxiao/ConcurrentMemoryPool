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
			_idSpanMap.erase(span->_pageId);
			Span* newSpan = new Span;   

			newSpan->_pageId = span->_pageId;
			newSpan->_n = kpages;

			span->_n -= kpages;
			span->_pageId += kpages;
			_pageLists[span->_n].PushFront(span);

			//�������PageCache�б����ҳ��ֻ��Ҫ������βҳ����ӳ�伴��
			_idSpanMap[span->_pageId] = span;
			_idSpanMap[span->_pageId + span->_n - 1] = span;


			//���ֳ���CentralCache��spanÿ��ҳ����ָ��ӳ������
			for (size_t i = 0; i < newSpan->_n; i++)
			{
				_idSpanMap[newSpan->_pageId + i] = newSpan;
			}

			return newSpan;
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

//�������ַת��Ϊҳ�ţ�Ѱ�Ҷ�Ӧ��span
Span* PageCache::ObjAddressToSpan(void* obj)
{
	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT;
	auto ret = _idSpanMap.find(id);
	if (ret != _idSpanMap.end())
	{
		return ret->second;
	}
	//һ�������ҵ��ģ�����Ҳ�����˵�������˴���
	else
	{
		assert(false);
		return nullptr;
	}
}

//��CentralCache�����Ŀ���span��ҳ�����ڵ�span�ϲ�
void PageCache::ReleaseSpan(Span* span)
{
	//�Ƚ�С�ڴ�span��һҳ������ҳ�źϲ�
	while (1)
	{
		PAGE_ID id = span->_pageId;
		auto ret = _idSpanMap.find(id - 1);
		//���ǰһ��span�����ڣ����˳�
		if (ret == _idSpanMap.end()) break;

		Span* prevSpan = ret->second;
		//���ǰһ��span���ڱ�ʹ�ã����˳�
		//�˴�����ʹ��useCount,��Ϊ��span�ձ������������ʱΪ0�����ǲ��ܽ���ϲ�
		------------------------------------------------
		if (prevSpan-> != 0) break;

		//�����ǰһ��span�ϲ��󳬹���PageCache�ܹҵ����Span,���˳�
		if (prevSpan->_n + span->_n > KPAGE - 1) break;

		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;
		delete prevSpan;
	}
	//���ڴ�spanҳ֮�������span�ϲ�
	while (1)
	{
		PAGE_ID id = span->_pageId;
		auto ret = _idSpanMap.find(id + span->_n);

		//���ǰһ��span�����ڣ����˳�
		if (ret == _idSpanMap.end()) break;

		Span* nextSpan = ret->second;
		//���ǰһ��span���ڱ�ʹ�ã����˳�
		//�˴�����ʹ��useCount,��Ϊ��span�ձ������������ʱΪ0�����ǲ��ܽ���ϲ�
		------------------------------------------------
		if (nextSpan->_useCount != 0) break;

		//�����ǰһ��span�ϲ��󳬹���PageCache�ܹҵ����Span,���˳�
		if (nextSpan->_n + span->_n > KPAGE - 1) break;

		span->_n += nextSpan->_n;
		delete nextSpan;
	}

	//�ϲ���
}
