#include <cassert>
#include <algorithm>

#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocate(size_t sizeByte)
{
	//������ڴ����Ϸ�
	assert(sizeByte > 0 && sizeByte <= MAX_BYTES);
	size_t alignSize = SizeClass::RoundUp(sizeByte);
	size_t index = SizeClass::Index(sizeByte);
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
	size_t num = std::min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
	if (num == _freeLists[index].MaxSize()) _freeLists[index].MaxSize()++;

	void* start = nullptr;
	void* end = nullptr;
	size_t actual_num = CentralCache::GetInstance()->FetchRangeObj(start,end,num,size);
	//���actual_numΪ0����startӦ��Ϊnullptr����ʾ��ȡ�ڴ��ʧ�ܣ�ֱ�ӷ���start
	if(actual_num > 1)
		_freeLists[index].PushFront(NextObj(start), end,actual_num-1);
	
	NextObj(start) = nullptr;
	return start;
}
