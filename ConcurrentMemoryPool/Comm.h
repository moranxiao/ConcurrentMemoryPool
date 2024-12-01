#pragma once
#include <cassert>
#include <mutex>

#ifdef _WIN32
#include <Windows.h>
#else
//Linux�������ڴ�ϵͳ���ÿ�
#endif

#ifdef _WIN64
typedef long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#endif
//���û������ڴ�С�ڵ���MAX_BYTESʱ���ڴ������
//����MAX_BYTES��ֱ����ϵͳ��PageCache����
	
//�ڴ���������������ֽ���
static const size_t MAX_BYTES = 256 * 1024;
//ThreadCache��CentralCache��Ͱ�ĸ���
static const size_t FREELISTS_NUM = 208;
//PageCache��Ͱ��
static const size_t KPAGE = 129;
//һ��Page�Ĵ�С:2^13
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
//linux��brk��mmp��
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
	void PushFront(void* begin, void* end,size_t n)
	{
		NextObj(end) = _freeList;
		_freeList = begin;
		_size += n;
	}
	void* PopFront()
	{
		void* pop_obj = _freeList;
		_freeList = NextObj(_freeList);
		_size--;
		return pop_obj;
	}

	bool Empty()
	{
		return _size == 0;
	}
	size_t& MaxSize()
	{
		return _maxSize;
	}
private:
	void* _freeList = nullptr;
	size_t _maxSize = 1; //��centralcache�����ڴ������������������ȡС���ڴ���
	size_t _size = 0; //����ڵ����
};

class SizeClass {
public:
	static size_t _RoundUp(size_t sizeByte,size_t align)
	{
		return  (sizeByte + align - 1) & ~(align - 1);
	}
	static size_t RoundUp(size_t sizeByte)
	{
		if (sizeByte <= 128) return _RoundUp(sizeByte, 8);
		else if(sizeByte <= 1024) return _RoundUp(sizeByte, 16);
		else if (sizeByte <= 8*1024) return _RoundUp(sizeByte, 128);
		else if (sizeByte <= 64*1024) return _RoundUp(sizeByte, 1024);
		else if (sizeByte <= MAX_BYTES) return _RoundUp(sizeByte, 8*1024);
		else {
			//�����ǳ���MAX_BYTES�����
			return -1;
		}
	}

	static size_t _Index(size_t sizeByte,size_t align_shift)
	{
		return ((sizeByte + (1 << align_shift) - 1) >> align_shift) - 1;
	}
	static size_t Index(size_t sizeByte)
	{
		assert(sizeByte <= MAX_BYTES);
		/*
		* threadCache��ÿ��Ͱ�������ڴ���С��һ
		* [1,128] ����16��Ͱ������Ͱ֮���ڴ��С���8B
		* [129,1024] ��56��Ͱ������Ͱ֮���ڴ��С���16B
		* [1025,8*1024] ��56��Ͱ������Ͱ֮���ڴ��С���128B
		* [8*1024+1,64*1024] ��56��Ͱ������Ͱ֮���ڴ��С���1024B
		* [64*1024+1,256*1024] ��24��Ͱ������Ͱ֮���ڴ��С���8*1024B
		*/
		static const int group_array[] = { 16,56,56,56,24 };
		if (sizeByte <= 128) return _Index(sizeByte, 3);
		else if (sizeByte <= 1024) {
			return _Index(sizeByte - 128, 4) + group_array[0];
		}
		else if (sizeByte <= 8 * 1024) {
			return _Index(sizeByte - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (sizeByte <= 64 * 1024) {
			return _Index(sizeByte - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
		}
		else if (sizeByte <= 256 * 1024) {
			return _Index(sizeByte - 64 * 1024, 13) + group_array[3] + group_array[2] + group_array[1] + group_array[0];
		}
		else assert(false);
		return -1;
	}

	//threadcacheһ����centralcacheҪ��obj����������
	static size_t NumMoveSize(size_t sizeByte)
	{
		assert(sizeByte > 0);
		//����������
		//С����һ��������������޸�
		//�����һ��������������޵�
		size_t num = MAX_BYTES / sizeByte;
		if (num < 2) num = 2;
		else if (num > 512) num = 512;
		return num;
	}

	static size_t NumMovePage(size_t sizeByte)
	{
		size_t num = NumMoveSize(sizeByte);
		size_t pages = num * sizeByte;
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