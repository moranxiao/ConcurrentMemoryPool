
#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;

//��ȡһЩС���ڴ�
size_t CentralCache::FetchRangeObj(void*& begin, void*& end, size_t batch, size_t size)
{
	size_t index = SizeClass::Index(size);
	
	_spanLists[index].Mutex()->lock();
	Span* span = GetOneSpan(index,size);
	assert(span);
	assert(span->_freeList != nullptr);
	
	//��span��С���ڴ��У�ѡȡһ����
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

//��spanList�ϻ�ȡһ�����п���С���ڴ��span
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
	//�ߵ�����˵��û�п���Span�ˣ���Ҫ��PageCacheҪ
	
	PageCache::GetInstance()->Mutex()->lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	span->_isUse = true;
	PageCache::GetInstance()->Mutex()->unlock();
	
	//��ȡ��PageCache�õ��Ĵ���ڴ����β��ַ
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	//������ڴ��з�Ϊһ��һ��С�ڴ棬�����ӵ�freeList��
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

//��ThreadCache�ͷŵ�С���ڴ����¹ҵ�Span�ϣ�����spanС���ڴ涼���黹ʱ�����ͷŵ�PageCache��
void CentralCache::ReleaseListToSpans(void* begin,size_t size)
{
	size_t index = SizeClass::Index(size);
	_spanLists[index].Mutex()->lock();
	//��ÿ��С�ڴ�ͨ��ӳ���ҵ����Ӧ��span��������ȥ
	void* next = NextObj(begin);
	while (begin != nullptr)
	{
		Span* span = PageCache::GetInstance()->ObjAddressToSpan(begin);
		//ͷ����span��freeList��
		NextObj(begin) = span->_freeList;
		span->_freeList = begin;
		span->_useCount--;
		begin = next;
		next = NextObj(begin);
		
		//���span������С���ڴ��Ϊ��ʹ��������յ�PageCache��
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