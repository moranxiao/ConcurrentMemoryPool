
#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocate(size_t size)
{
	//申请的内存必须合法
	assert(size > 0 && size <= MAX_BYTES);
	size_t alignSize = SizeClass::RoundUp(size);
	size_t index = SizeClass::Index(size);
	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].PopFront();
	}
	//链表中没有空闲内存需要向CentralCache申请
	else
	{
		return FetchFromCentralCache(index, alignSize);
	}
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	/* 慢启动
	* 每个桶有自己的maxSize，表示申请过多少次，每次成功申请都会增加其值
	* 要申请的空闲内存块数 = min(maxSize,MAX_BYTES/size)
	* MAX_BYTES/size:小对象申请的上限大，大对象申请的上限小
	*/
	size_t batch = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
	if (batch == _freeLists[index].MaxSize()) _freeLists[index].MaxSize()++;

	void* start = nullptr;
	void* end = nullptr;
	size_t actual_batch = CentralCache::GetInstance()->FetchRangeObj(start,end, batch,size);
	//如果actual_num为0，则表示内存获取失败，
	// 1.要么申请内存没申请到，会抛异常的
	// 2.要么程序逻辑错误，应该断言检查
	assert(actual_batch > 0);

	if (actual_batch > 1)
	{
		_freeLists[index].PushRangeFront(NextObj(start), end, actual_batch - 1);
	}
	else
	{
		assert(start == end);
	}
	return start;
}

//释放对象内存给ThreadCache
void ThreadCache::FreeObj(void* ptr, size_t size)
{
	assert(size > 0 && size <= MAX_BYTES);
	size_t index = SizeClass::Index(size);
	_freeLists[index].PushFront(ptr);
	
	//如果现在ThreadCache中自由链表空闲内存过多就将其打包释放给CentralCache
	//此实现细节只考虑了自由链表过长的情况，还可以加上自由链表所含闲置内存达到一个阈值时触发
	if (_freeLists[index].MaxSize() <= _freeLists[index].Size())
	{
		ListTooLong(_freeLists[index], size);
	}
}

void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* begin;
	void* end;
	list.PopRangeFront(begin, end, list.MaxSize());
	CentralCache::GetInstance()->ReleaseListToSpans(begin, size);
}
