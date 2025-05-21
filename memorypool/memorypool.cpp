
#include "memorypool.h"

MemoryPool::MemoryPool()
{
}

void MemoryPool::init(int size)
{
	slot_size = size;
	currentBlock = currentSlot = lastSlot = freeSlot = NULL;
}

MemoryPool::~MemoryPool()
{
	Slot *curr = currentBlock;//指向当前块
	while (curr)
	{
		Slot *prev = curr->next;//备份下一块
		free(reinterpret_cast<void *>(curr));
		curr = prev;
	}
}

inline size_t MemoryPool::padPointer(char *p, size_t align)
{
	uintptr_t result = reinterpret_cast<uintptr_t>(p);
	return ((align - result) % align);
}

Slot *MemoryPool::allocateBlock()
{
	char *newBlock = NULL;
	while (!(newBlock = reinterpret_cast<char *>(malloc(BlockSize)))){};
	char *body = newBlock + sizeof(Slot);
	size_t bodyPadding = padPointer(body, static_cast<size_t>(slot_size));
	Slot *useSlot;
	{
		MutexLockGuard lock(m_other);
		//新分配的块头插到当前块链表中
		reinterpret_cast<Slot *>(newBlock)->next = currentBlock;
		currentBlock = reinterpret_cast<Slot *>(newBlock);
		currentSlot = reinterpret_cast<Slot *>(body + bodyPadding);
		lastSlot = reinterpret_cast<Slot *>(newBlock + BlockSize - slot_size);
		useSlot = currentSlot;
		currentSlot += (slot_size >> 3);
	}
	return useSlot;
}

//处理无空闲槽位的情况
Slot *MemoryPool::nofree_solve()
{
	if (currentSlot >= lastSlot)
		return allocateBlock();
	Slot *useSlot;
	{
		MutexLockGuard lock(m_other);
		useSlot = currentSlot;
		currentSlot += (slot_size >> 3);
	}
	return useSlot;
}

//分配内存槽位
Slot *MemoryPool::allocate()
{
	if (freeSlot)
	{
		{
			MutexLockGuard lock(m_freeSlot);
			if (freeSlot)
			{
				Slot *result = freeSlot;
				freeSlot = freeSlot->next;
				return result;
			}
		}
	}
	return nofree_solve();
}

//释放内存槽位，将其加入空闲链表
inline void MemoryPool::deallocate(Slot *p)
{
	if (p)
	{
		MutexLockGuard lock(m_freeSlot);
		p->next = freeSlot;//从空闲槽位的头部取Slot作为维护链表
		/*使用中：100%用于用户数据
		释放后：只用指针大小的空间作为管理开销*/
		freeSlot = p;
	}
}

//根据大小分配内存
void *use_memory(size_t size)
{
	if (!size)
		return nullptr;
	if (size > 512)
		return malloc(size);
	//size + 7:向上取整到8的倍数
	return reinterpret_cast<void *>(get_memorypool(((size + 7) >> 3) - 1).allocate());
}

//根据大小释放指定内存
void free_memory(size_t size, void *p)
{
	if (!p)
		return;
	if (size > 512)
	{
		free(p);
		return;
	}
	get_memorypool(((size + 7) >> 3) - 1).deallocate(reinterpret_cast<Slot *>(p));
}

//分配内存池
void init_memorypool()
{
	for (int i = 0; i < 64; ++i)
		get_memorypool(i).init((i + 1) << 3);//分配不同槽位的内存8B到512B
}

// 0-63槽位的内存池
MemoryPool &get_memorypool(int id)
{
	static MemoryPool memorypool[64];
	return memorypool[id];
}