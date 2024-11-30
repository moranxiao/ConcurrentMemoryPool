
#include "CentralCache.h"



size_t CentralCache::FetchRangeObj(void* begin, void* end, size_t batch, size_t size)
{
	size_t index = SizeClass::Index(size);
	
	Span* span = GetOneSpan(index);
	
	
}

Span* CentralCache::GetOneSpan(size_t index)
{
	Span* it = _spanLists[index].Begin();
	while (it != _spanLists[index].End())
	{
		if (it->_freeList != nullptr)
		{
			return it;
		}
	}
	//�ߵ�����˵��û�п���Span�ˣ���Ҫ��PageCacheҪ

}
