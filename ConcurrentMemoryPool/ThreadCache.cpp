
#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocate(size_t size)
{
	//������ڴ����Ϸ�
	assert(size > 0 && size <= MAX_BYTES);
	size_t alignSize = SizeClass::RoundUp(size);
	size_t index = SizeClass::Index(size);
	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].PopFront();
			
	}
	//������û�п����ڴ���Ҫ��CentralCache����
	else
	{
		return FetchFromCentralCache(index, alignSize);
	}
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	/* ������
	* ÿ��Ͱ���Լ���maxSize����ʾ��������ٴΣ�ÿ�γɹ����붼��������ֵ
	* Ҫ����Ŀ����ڴ���� = min(maxSize,MAX_BYTES/size)
	* MAX_BYTES/size:С������������޴󣬴�������������С
	*/
	size_t batch = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
	if (batch == _freeLists[index].MaxSize()) _freeLists[index].MaxSize()++;

	void* start = nullptr;
	void* end = nullptr;
	size_t actual_batch = CentralCache::GetInstance()->FetchRangeObj(start,end, batch,size);
	//���actual_numΪ0����startӦ��Ϊnullptr����ʾ��ȡ�ڴ��ʧ�ܣ�ֱ�ӷ���start
	if(actual_batch > 1)
		_freeLists[index].PushRangeFront(NextObj(start), end,actual_batch -1);
	
	NextObj(start) = nullptr;
	return start;
}

//�ͷŶ����ڴ��ThreadCache
void ThreadCache::FreeObj(void* ptr, size_t size)
{
	assert(size > 0 && size <= MAX_BYTES);
	size_t index = SizeClass::Index(size);
	_freeLists[index].PushFront(ptr);
	
	//�������ThreadCache��������������ڴ����ͽ������ͷŸ�CentralCache
	//��ʵ��ϸ��ֻ�����������������������������Լ��������������������ڴ�ﵽһ����ֵʱ����
	if (_freeLists[index].MaxSize() <= _freeLists[index].Size())
	{
		void* begin;
		void* end;
		_freeLists[index].PopRangeFront(begin, end, _freeLists[index].MaxSize());
		CentralCache::GetInstance()->ReleaseListToSpans(begin,size);
	}
}
