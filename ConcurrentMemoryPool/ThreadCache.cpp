#include <cassert>
#include <algorithm>

#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocate(size_t sizeByte)
{
	//申请的内存必须合法
	assert(sizeByte > 0 && sizeByte <= MAX_BYTES);
	size_t alignSize = SizeClass::RoundUp(sizeByte);
	size_t index = SizeClass::Index(sizeByte);
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
	//如果actual_num为0，则start应该为nullptr，表示获取内存块失败，直接返回start
	if(actual_batch > 1)
		_freeLists[index].PushFront(NextObj(start), end,actual_batch -1);
	
	NextObj(start) = nullptr;
	return start;
}
