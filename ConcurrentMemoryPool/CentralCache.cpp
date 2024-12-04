
#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;

//获取一些小块内存
size_t CentralCache::FetchRangeObj(void*& begin, void*& end, size_t batch, size_t size)
{
	size_t index = SizeClass::Index(size);
	
	_spanLists[index].Mutex()->lock();
	Span* span = GetOneSpan(index,size);
	assert(span);
	assert(span->_freeList != nullptr);
	
	//从span的小块内存中，选取一部分
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
	span->_useCount += actualNum;

	_spanLists[index].Mutex()->unlock();
	return actualNum;
}

//从spanList上获取一个含有空闲小块内存的span
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
	span->_isUse = true;
	PageCache::GetInstance()->Mutex()->unlock();
	
	//获取从PageCache得到的大块内存的首尾地址
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	//将大块内存切分为一块一块小内存，并链接到freeList上
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

//将ThreadCache释放的小块内存重新挂到Span上，当此span小块内存都被归还时，再释放到PageCache中
void CentralCache::ReleaseListToSpans(void* begin,size_t size)
{
	size_t index = SizeClass::Index(size);
	_spanLists[index].Mutex()->lock();
	//将每块小内存通过映射找到其对应的span，并挂上去
	void* next = NextObj(begin);
	while (begin != nullptr)
	{
		Span* span = PageCache::GetInstance()->ObjAddressToSpan(begin);
		//头插入span的freeList上
		NextObj(begin) = span->_freeList;
		span->_freeList = begin;
		span->_useCount--;
		begin = next;
		next = NextObj(begin);
		
		//如果span的所有小块内存均为被使用则将其回收到PageCache中
		if (span->_useCount == 0)
		{
			_spanLists[index].Erase(span);
			_spanLists[index].Mutex()->unlock();
			PageCache::GetInstance()->Mutex()->lock();
			PageCache::GetInstance()->ReleaseSpan(span);
			PageCache::GetInstance()->Mutex()->unlock();
			_spanLists[index].Mutex()->lock();

		}
	}

	_spanLists[index].Mutex()->unlock();
}