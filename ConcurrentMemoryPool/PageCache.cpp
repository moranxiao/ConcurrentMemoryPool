#include "PageCache.h"
#include "CentralCache.h"

PageCache PageCache::_sInst;

Span* PageCache::NewSpan(size_t kpages)
{
	assert(kpages >= 1);

	if (kpages >= KPAGE)
	{
		Span* newSpan =  _spanPool.New();
		void* ptr = SystemAlloc(kpages);
		newSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		newSpan->_n = kpages;
		_idSpanMap.set(newSpan->_pageId,newSpan);
		return newSpan;
	}
	//����һ��Ͱ��û��Span
	if (!_pageLists[kpages].Empty())
	{
		Span* span = _pageLists[kpages].PopFront();
		//���ֳ���CentralCache��spanÿ��ҳ����ָ��ӳ������
		for (size_t i = 0; i < span->_n; i++)
		{
			_idSpanMap.set(span->_pageId + i, span);
		}
		span->_isUse = true;
		return span;
	}
	
	for (size_t i = kpages + 1; i < KPAGE; i++)
	{
		//�����kpages���Ͱ����span��ֱ�ӷָ�span
		if (!_pageLists[i].Empty())
		{
			Span* span = _pageLists[i].PopFront();
			Span* newSpan = _spanPool.New();
			newSpan->_pageId = span->_pageId;
			newSpan->_n = kpages;

			span->_n -= kpages;
			span->_pageId += kpages;
			_pageLists[span->_n].PushFront(span);

			//�������PageCache�б����ҳ��ֻ��Ҫ������βҳ����ӳ�伴��
			_idSpanMap.set(span->_pageId, span);
			_idSpanMap.set(span->_pageId + span->_n - 1, span);

			//���ֳ���CentralCache��spanÿ��ҳ����ָ��ӳ������
			for (size_t i = 0; i < newSpan->_n; i++)
			{
				_idSpanMap.set(newSpan->_pageId + i, newSpan);
			}

			return newSpan;
		}
	}
	//�����kpages���Ͱ��û��span������ϵͳ����

	//��ϵͳ������ڴ�������ַ��ֱ��ӳ��Ϊҳ��
	//ֻҪ��֤PAGE��ϵͳҳ����һ����С����

	void* ptr = SystemAlloc(KPAGE-1);
	Span* span = _spanPool.New();
	span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	span->_n = KPAGE-1;
	_pageLists[span->_n].PushFront(span);
	return NewSpan(kpages);
}




//�������ַת��Ϊҳ�ţ�Ѱ�Ҷ�Ӧ��span
Span* PageCache::MapObjToSpan(void* obj)
{
	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT;
	//std::unique_lock<std::mutex> uniqueMtx(*PageCache::GetInstance()->Mutex());
	//auto ret = _idSpanMap.find(id);
	//if (ret != _idSpanMap.end())
	//{
	//	return ret->second;
	//}
	Span* span =(Span*)_idSpanMap.get(id);
	if (span != nullptr)
		return span;
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
	//����Ǳ�pagecache�ܴ洢�����span��Ҫ���span
	//������ֱ�ӷ���ͨ��ϵͳ�����ڴ��
	if (span->_n >= KPAGE)
	{
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		_spanPool.Delete(span);
		return;
	}
	

	//�Ƚ�С�ڴ�span��һҳ������ҳ�ϲ�
	while (1)
	{
		PAGE_ID id = span->_pageId;
		//auto ret = _idSpanMap.find(id - 1);
		////���ǰһ��span�����ڣ����˳�
		//if (ret == _idSpanMap.end()) break;

		//Span* prevSpan = ret->second;


		Span* prevSpan = (Span*)_idSpanMap.get(id - 1);

		if (prevSpan == nullptr) break;

		//���ǰһ��span���ڱ�ʹ�ã����˳�
		//�˴�����ʹ��useCount,��Ϊ��span�ձ������������ʱΪ0�����ǲ��ܽ���ϲ�
		if (prevSpan->_isUse) break;

		//�����ǰһ��span�ϲ��󳬹���PageCache�ܹҵ����Span,���˳�
		if (prevSpan->_n + span->_n > KPAGE - 1) break;

		_pageLists[prevSpan->_n].Erase(prevSpan);
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;
		_spanPool.Delete(prevSpan);

	}
	//���ڴ�spanҳ֮�������span�ϲ�
	while (1)
	{
		PAGE_ID id = span->_pageId;
		//auto ret = _idSpanMap.find(id + span->_n);

		////�����һ��span�����ڣ����˳�
		//if (ret == _idSpanMap.end()) break;

		//Span* nextSpan = ret->second;

		Span* nextSpan = (Span*)_idSpanMap.get(id + span->_n);
		if (nextSpan == nullptr) break;

		//���ǰһ��span���ڱ�ʹ�ã����˳�
		//�˴�����ʹ��useCount,��Ϊ��span�ձ������������ʱΪ0�����ǲ��ܽ���ϲ�
		if (nextSpan->_isUse) break;

		//������һ��span�ϲ��󳬹���PageCache�ܹҵ����Span,���˳�
		if (nextSpan->_n + span->_n > KPAGE - 1) break;

		_pageLists[nextSpan->_n].Erase(nextSpan);
		span->_n += nextSpan->_n;
		_spanPool.Delete(nextSpan);
	}

	//�ϲ���Ҫ�޸�pageId��span����������Ȼ���ҵ�֮ǰ���ͷŵ�span

	//_idSpanMap[span->_pageId] = span;
	//_idSpanMap[span->_pageId + span->_n - 1] = span;

	_idSpanMap.set(span->_pageId, span);
	_idSpanMap.set(span->_pageId + span->_n - 1, span);

	span->_isUse = false;
	_pageLists[span->_n].PushFront(span);
}
