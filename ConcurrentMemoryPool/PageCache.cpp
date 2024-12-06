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
		_idSpanMap[newSpan->_pageId] = newSpan;
		return newSpan;
	}

	//检查第一个桶有没有Span
	if (!_pageLists[kpages].Empty())
	{
		Span* span = _pageLists[kpages].PopFront();
		//将分出给CentralCache的span每个页与其指针映射起来
		for (size_t i = 0; i < span->_n; i++)
		{
			_idSpanMap[span->_pageId + i] = span;
		}
		return span;
	}
	
	for (size_t i = kpages + 1; i < KPAGE; i++)
	{
		//如果比kpages大的桶中有span则直接分割span
		if (!_pageLists[i].Empty())
		{
			Span* span = _pageLists[i].PopFront();
			_idSpanMap.erase(span->_pageId);
			Span* newSpan = _spanPool.New();
			newSpan->_pageId = span->_pageId;
			newSpan->_n = kpages;

			span->_n -= kpages;
			span->_pageId += kpages;
			_pageLists[span->_n].PushFront(span);

			//如果是在PageCache中保存的页，只需要将其首尾页进行映射即可
			_idSpanMap[span->_pageId] = span;
			_idSpanMap[span->_pageId + span->_n - 1] = span;


			//将分出给CentralCache的span每个页与其指针映射起来
			for (size_t i = 0; i < newSpan->_n; i++)
			{
				_idSpanMap[newSpan->_pageId + i] = newSpan;
			}

			return newSpan;
		}
	}
	//如果比kpages大的桶中没有span，则向系统申请

	//像系统申请的内存根据其地址，直接映射为页号
	//只要保证PAGE和系统页面是一样大小就行

	void* ptr = SystemAlloc(KPAGE-1);
	Span* span = _spanPool.New();
	span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	span->_n = KPAGE-1;
	_pageLists[span->_n].PushFront(span);
	return NewSpan(kpages);
}

//将对象地址转换为页号，寻找对应的span
Span* PageCache::MapObjToSpan(void* obj)
{
	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT;
	std::unique_lock<std::mutex> uniqueMtx(*PageCache::GetInstance()->Mutex());
	auto ret = _idSpanMap.find(id);
	if (ret != _idSpanMap.end())
	{
		return ret->second;
	}
	//一定是能找到的，如果找不到则说明发生了错误
	else
	{
		assert(false);
		return nullptr;
	}
}

//将CentralCache传来的空闲span与页号相邻的span合并
void PageCache::ReleaseSpan(Span* span)
{
	//如果是比pagecache能存储的最大span还要大的span
	//则其是直接分配通过系统分配内存的
	if (span->_n >= KPAGE)
	{
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		_idSpanMap.erase(span->_pageId);
		_spanPool.Delete(span);
		return;
	}
	

	//先将小于此span第一页的相邻页合并
	while (1)
	{
		PAGE_ID id = span->_pageId;
		auto ret = _idSpanMap.find(id - 1);
		//如果前一个span不存在，则退出
		if (ret == _idSpanMap.end()) break;

		Span* prevSpan = ret->second;
		//如果前一个span还在被使用，则退出
		//此处不能使用useCount,因为当span刚被申请出来，此时为0，但是不能将其合并
		if (prevSpan->_isUse) break;

		//如果与前一个span合并后超过了PageCache能挂的最大Span,则退出
		if (prevSpan->_n + span->_n > KPAGE - 1) break;

		_pageLists[prevSpan->_n].Erase(prevSpan);
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;
		_spanPool.Delete(prevSpan);

	}
	//将在此span页之后的相邻span合并
	while (1)
	{
		PAGE_ID id = span->_pageId;
		auto ret = _idSpanMap.find(id + span->_n);

		//如果前一个span不存在，则退出
		if (ret == _idSpanMap.end()) break;

		Span* nextSpan = ret->second;
		//如果前一个span还在被使用，则退出
		//此处不能使用useCount,因为当span刚被申请出来，此时为0，但是不能将其合并
		if (nextSpan->_isUse) break;

		//如果与前一个span合并后超过了PageCache能挂的最大Span,则退出
		if (nextSpan->_n + span->_n > KPAGE - 1) break;

		_pageLists[nextSpan->_n].Erase(nextSpan);
		span->_n += nextSpan->_n;
		_spanPool.Delete(nextSpan);
	}

	//合并后
	span->_isUse = false;
	span->_freeList = nullptr;
	_pageLists[span->_n].PushFront(span);
}
