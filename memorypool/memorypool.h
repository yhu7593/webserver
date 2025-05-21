#pragma once

#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <stdint.h>
#include <stdarg.h>
#include <utility>
#include "../lock/locker.h"
#include <functional>

#define BlockSize 4096

struct Slot{
	Slot* next;
};

class MemoryPool{
private:
	int slot_size;	//每个槽的大小
		
	Slot* currentBlock;//当前内存块
	Slot* currentSlot;//当前可用槽位
	Slot* lastSlot;//当前块的最后一个槽位
	Slot* freeSlot;//已释放的空闲槽位链表
	
	locker m_freeSlot;
	locker m_other;
	size_t padPointer(char* p,size_t align);//内存对齐
	Slot* allocateBlock();//分配内存块
	Slot* nofree_solve();//处理无空闲槽位的情况

public:
	MemoryPool();
	~MemoryPool();
	void init(int size);	
	Slot* allocate();//分配内存槽位
	void deallocate(Slot* p);//释放内存槽位，将其加入空闲链表

};

//void *operator new(size_t);
//void operator delete(void *p,size_t size);
void init_memorypool();
void* use_memory(size_t size);//根据大小分配内存
void free_memory(size_t size,void *p);//释放指定大小的内存
MemoryPool& get_memorypool(int id);

template<class T,class... Args>
T* newElement(Args&&... args){
	T *p;
	if(p=reinterpret_cast<T *>(use_memory(sizeof(T))))
		new(p)T(std::forward<Args>(args)...);
	return p;
}

template<class T>
void deleteElement(T* p){
	if(p)
		p->~T();
	free_memory(sizeof(T),reinterpret_cast<void *>(p));
}