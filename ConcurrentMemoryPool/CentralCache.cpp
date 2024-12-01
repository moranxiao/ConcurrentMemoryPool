
#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;

size_t CentralCache::FetchRangeObj(void*& begin, void*& end, size_t batch, size_t size)
{
	size_t index = SizeClass::Index(size);
	
	_spanLists[index].Mutex()->lock();
	Span* span = GetOneSpan(index,size);
	assert(span);
	assert(span->_freeList != nullptr);
	

	void* cur = span->_freeList;
	batch--;
	size_t actualNum = 1;
	begin = cur;
	while (batch > 0 && NextObj(cur) != nullptr)
	{
		cur = NextObj(cur);
		actualNum++;
		batch--;
	}
	span->_freeList = NextObj(cur);
	NextObj(cur) = nullptr;
	end = cur;
	
	_spanLists[index].Mutex()->unlock();
	return actualNum;
}

Span* CentralCache::GetOneSpan(size_t index,size_t size)
{
	Span* it = _spanLists[index].Begin();
	while (it != _spanLists[index].End())
	{
		if (it->_freeList != nullptr)
		{
			return it;
		}
		it = it->_next;
	}
	_spanLists[index].Mutex()->unlock();
	//走到这里说明没有空闲Span了，需要向PageCache要
	
	PageCache::GetInstance()->Mutex()->lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	PageCache::GetInstance()->Mutex()->unlock();
	
	//切分span的freeList为一个一个小块内存
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;
	
	while (start < end)
	{
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}
	NextObj(tail) = nullptr;
	_spanLists[index].Mutex()->lock();
	_spanLists[index].PushFront(span);
	return span;
}
