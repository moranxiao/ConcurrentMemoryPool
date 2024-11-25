#pragma once

#include <cstdlib>
#include <vector>
#ifdef _WIN64
#include <Windows.h>
#else

#endif

template<class T>
class ObjectPool {
	const static size_t _allocPageNum = 16;
	const static size_t _mallocSize = 1024 * 128;
	static void* SystemAlloc(size_t kpage)
	{
#ifdef _WIN64
		void* ptr = VirtualAlloc(0, kpage * (1 << 12), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//linux下brk和mmap等
#endif
		if (ptr == nullptr)
			throw std::bad_alloc();
		return ptr;
	}
public:
	T* New()
	{
		T* obj;
		//如果释放空间链表中有闲置内存，则直接获取闲置内存
		if (_freeList)
		{
			obj = (T*)_freeList;
			_freeList = *(void**)_freeList;
		}
		else
		{
			size_t size = sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);
			//如果大块内存剩余量不够分配，则重新开辟
			if (_remainSize < sizeof(T))
			{
				//_memory = (char*)malloc(_mallocSize); //一次申请128KB内存
				//if (_memory == nullptr)
				//	throw std::bad_alloc();
				//_remainSize = _mallocSize;
				_memory = (char*)SystemAlloc(_allocPageNum);
				_remainSize = _allocPageNum << 12;
			}
			obj = (T*)_memory;
			_memory += size;
			_remainSize -= size;
		}
		obj = new(obj)T;
		return obj;
	}
	void Delete(T* obj)
	{
		obj->~T(); //析构对象
		*(void**)obj = _freeList; //头插
		_freeList = (void*)obj;
	}
private:
	char* _memory = nullptr; //一次申请一大块内存
	size_t _remainSize = 0; //大块内存中的剩余量
	void* _freeList = nullptr; //释放的空闲内存，用链表组织起来
};


struct TreeNode
{
	int _val;
	TreeNode* _left;
	TreeNode* _right;

	TreeNode()
		:_val(0)
		, _left(nullptr)
		, _right(nullptr)
	{}
};

void TestObjectPool()
{
	// 申请释放的轮次
	const size_t Rounds = 5;

	// 每轮申请释放多少次
	const size_t N = 100000;

	std::vector<TreeNode*> v1;
	v1.reserve(N);

	size_t begin1 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v1.push_back(new TreeNode);
		}
		for (int i = 0; i < N; ++i)
		{
			delete v1[i];
		}
		v1.clear();
	}

	size_t end1 = clock();

	std::vector<TreeNode*> v2;
	v2.reserve(N);

	ObjectPool<TreeNode> TNPool;
	size_t begin2 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v2.push_back(TNPool.New());
		}
		for (int i = 0; i < N; ++i)
		{
			TNPool.Delete(v2[i]);
		}
		v2.clear();
	}
	size_t end2 = clock();

	cout << "new cost time:" << end1 - begin1 << endl;
	cout << "object pool cost time:" << end2 - begin2 << endl;
}