#pragma once
#include <cassert>
#include <mutex>
#include <algorithm>
#include <unordered_map>

#ifdef _WIN32
#include <Windows.h>
#else
//Linux下申请内存系统调用库
#endif

#ifdef _WIN64
typedef long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#endif
//当用户申请内存小于等于MAX_BYTES时向内存池申请
//大于MAX_BYTES则直接向系统或PageCache申请
	
//内存池最大申请可申请字节数
static const size_t MAX_BYTES = 256 * 1024;
//ThreadCache和CentralCache中桶的个数
static const size_t FREELISTS_NUM = 208;
//PageCache的桶数
static const size_t KPAGE = 129;
//一个Page的大小:2^13
static const size_t PAGE_SHIFT = 13;

static void*& NextObj(void* obj)
{
	return *(void**)obj;
}

static void* SystemAlloc(size_t kpages)
{
	assert(kpages > 0);
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpages << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
//linux下brk，mmp等
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}


class FreeList {
public:
	void PushFront(void* obj)
	{
		NextObj(obj) = _freeList;
		_freeList = obj;
		_size++;
	}
	void PushRangeFront(void* begin, void* end,size_t n)
	{
		NextObj(end) = _freeList;
		_freeList = begin;
		_size += n;
	}
	void* PopFront()
	{
		void* obj = _freeList;
		_freeList = NextObj(_freeList);
		_size--;
		return obj;
	}
	void PopRangeFront(void*& begin,void*& end,size_t n)
	{
		assert(n <= _size);
		begin = _freeList;
		void* cur = _freeList;
		for (size_t i = 1; i < n; i++)
		{
			cur = NextObj(cur);
		}
		end = cur;
		NextObj(end) = nullptr;
		_freeList = NextObj(cur);
		_size -= n;
	}
	bool Empty()
	{
		return _size == 0;
	}
	size_t& MaxSize()
	{
		return _maxSize;
	}
	size_t Size()
	{
		return _size;
	}
private:
	void* _freeList = nullptr;
	size_t _maxSize = 1; //向centralcache申请内存次数，用于慢启动获取小块内存数
	size_t _size = 0; //链表节点个数
};

class SizeClass {
public:
	static size_t _RoundUp(size_t size,size_t align)
	{
		return  (size + align - 1) & ~(align - 1);
	}
	static size_t RoundUp(size_t size)
	{
		if (size <= 128) return _RoundUp(size, 8);
		else if(size <= 1024) return _RoundUp(size, 16);
		else if (size <= 8*1024) return _RoundUp(size, 128);
		else if (size <= 64*1024) return _RoundUp(size, 1024);
		else if (size <= MAX_BYTES) return _RoundUp(size, 8*1024);
		else {
			//这里是超过MAX_BYTES的情况
			return -1;
		}
	}

	static size_t _Index(size_t size,size_t align_shift)
	{
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
	}
	static size_t Index(size_t size)
	{
		assert(size <= MAX_BYTES);
		/*
		* threadCache中每个桶所含的内存块大小不一
		* [1,128] 中有16个桶，相邻桶之间内存大小相差8B
		* [129,1024] 有56个桶，相邻桶之间内存大小相差16B
		* [1025,8*1024] 有56个桶，相邻桶之间内存大小相差128B
		* [8*1024+1,64*1024] 有56个桶，相邻桶之间内存大小相差1024B
		* [64*1024+1,256*1024] 有24个桶，相邻桶之间内存大小相差8*1024B
		*/
		static const int group_array[] = { 16,56,56,56,24 };
		if (size <= 128) return _Index(size, 3);
		else if (size <= 1024) {
			return _Index(size - 128, 4) + group_array[0];
		}
		else if (size <= 8 * 1024) {
			return _Index(size - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (size <= 64 * 1024) {
			return _Index(size - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
		}
		else if (size <= 256 * 1024) {
			return _Index(size - 64 * 1024, 13) + group_array[3] + group_array[2] + group_array[1] + group_array[0];
		}
		else assert(false);
		return -1;
	}

	//threadcache一次向centralcache要的obj数量的上限
	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0);
		//慢启动策略
		//小对象一次批量申请的上限高
		//大对象一次批量申请的上限低
		size_t num = MAX_BYTES / size;
		if (num < 2) num = 2;
		else if (num > 512) num = 512;
		return num;
	}

	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t pages = num * size;
		pages >>= PAGE_SHIFT;
		if (pages < 1) pages = 1;
		return pages;
	}
};

struct Span {
	PAGE_ID _pageId = 0;
	size_t _n = 0;

	Span* _next = nullptr;
	Span* _prev = nullptr;

	void* _freeList = nullptr;
	size_t _useCount = 0;

};

class SpanList {
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}
	void Insert(Span* pos,Span* span)
	{
		if (span != nullptr && pos != nullptr)
		{
			pos->_prev->_next = span;
			span->_prev = pos->_prev;
			pos->_prev = span;
			span->_next = pos;
			_size++;
		}
	}
	void Erase(Span* pos)
	{
		if (pos != nullptr)
		{
			pos->_prev->_next = pos->_next;
			pos->_next->_prev = pos->_prev;
			_size--;
		}
	}
	
	void PushFront(Span* span)
	{
		Insert(_head->_next, span);
	}

	Span* PopFront()
	{
		if (!Empty())
		{
			Span* span = _head->_next;
			Erase(span);
			return span;
		}	
		return nullptr;
	}

	Span* Front()
	{
		if (!Empty())
		{
			return _head->_next;
		}
		return nullptr;
	}

	Span* Begin()
	{
		return _head->_next;
	}
	Span* End()
	{
		return _head;
	}
	std::mutex* Mutex()
	{
		return &_mtx;
	}
	bool Empty()
	{
		return _size == 0;
	}
private:
	Span* _head;
	size_t _size = 0;
	std::mutex _mtx;
};