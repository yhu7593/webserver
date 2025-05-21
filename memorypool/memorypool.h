#pragma once

#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <stdint.h>
#include <stdarg.h>
#include <utility>
#include "../lock/locker.h"
#include <functional>

#define BlockSize 16384 // 内存块大小
#define MAX_BYTES 4096  // 最大可分配字节数
#define POOL_SIZE  64   // 内存池大小  
struct Slot
{
    Slot *next;
};

class MemoryPool
{
private:
    int slot_size; // 每个槽的大小

    Slot *currentBlock; // 当前内存块
    Slot *currentSlot;  // 当前可用槽位
    Slot *lastSlot;     // 当前块的最后一个槽位
    Slot *freeSlot;     // 已释放的空闲槽位链表

    locker m_freeSlot;
    locker m_other;
    // 内存对齐函数
    size_t padPointer(char *p, size_t align)
    {
        uintptr_t result = reinterpret_cast<uintptr_t>(p);
        return ((align - result) % align);
    }

    // 分配内存块
    Slot *allocateBlock()
    {
        char *newBlock = NULL;
        while (!(newBlock = reinterpret_cast<char *>(malloc(BlockSize))))
        {
        };
        char *body = newBlock + sizeof(Slot);
        size_t bodyPadding = padPointer(body, static_cast<size_t>(slot_size));
        Slot *useSlot;
        {
            MutexLockGuard lock(m_other);
            // 新分配的块头插到当前块链表中
            reinterpret_cast<Slot *>(newBlock)->next = currentBlock;
            currentBlock = reinterpret_cast<Slot *>(newBlock);
            currentSlot = reinterpret_cast<Slot *>(body + bodyPadding);
            lastSlot = reinterpret_cast<Slot *>(newBlock + BlockSize - slot_size);
            useSlot = currentSlot;
            currentSlot += (slot_size >> 3);
        }
        return useSlot;
    }

    // 处理无空闲槽位的情况
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

public:
    MemoryPool() {}

    ~MemoryPool()
    {
        Slot *curr = currentBlock; // 指向当前块
        while (curr)
        {
            Slot *prev = curr->next; // 备份下一块
            free(reinterpret_cast<void *>(curr));
            curr = prev;
        }
    }

    void init(int size)
    {
        slot_size = size;
        currentBlock = currentSlot = lastSlot = freeSlot = NULL;
    }

    // 分配内存槽位
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

    // 释放内存槽位，将其加入空闲链表
    inline void MemoryPool::deallocate(Slot *p)
    {
        if (p)
        {
            MutexLockGuard lock(m_freeSlot);
            p->next = freeSlot; // 从空闲槽位的头部取Slot作为维护链表
            /*使用中：100%用于用户数据
            释放后：只用指针大小的空间作为管理开销*/
            freeSlot = p;
        }
    }
};

template <typename T, typename... Args>
class MemoryPoolManager
{
public:
    MemoryPoolManager()
    {
        init_memorypool();
    }
    ~MemoryPoolManager()
    {
        for (int i = 0; i < POOL_SIZE; ++i)
            get_memorypool(i).~MemoryPool();
    }
    // 分配内存
    T *newElement(Args &&...args)
    {
        T *p;
        if (p = reinterpret_cast<T *>(use_memory(sizeof(T))))
            new (p) T(std::forward<Args>(args)...);
        return p;
    }

    // 释放内存
    void deleteElement(T *p)
    {
        if (p)
            p->~T();
        free_memory(sizeof(T), reinterpret_cast<void *>(p));
    }

private:
    // 根据大小分配内存
    void *use_memory(size_t size)
    {
        if (!size)
            return nullptr;
        if (size > MAX_BYTES)
            return malloc(size);
        // size + 63:向上取整到64的倍数
        return reinterpret_cast<void *>(get_memorypool(((size + 7) >> 6) - 1).allocate());
    }

    // 根据大小释放指定内存
    void free_memory(size_t size, void *p)
    {
        if (!p)
            return;
        if (size > MAX_BYTES)
        {
            free(p);
            return;
        }
        get_memorypool(((size + 63) >> 6) - 1).deallocate(reinterpret_cast<Slot *>(p));
    }

    // 分配内存池
    void init_memorypool()
    {
        for (int i = 0; i < POOL_SIZE; ++i)
            get_memorypool(i).init((i + 1) << 6); // 分配不同槽位的内存64B到4096B
    }
    // 0-63槽位的内存池
    MemoryPool &get_memorypool(int id)
    {
        static MemoryPool memorypool[POOL_SIZE];
        return memorypool[id];
    }
};
