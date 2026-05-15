#include <not_implemented.h>
#include "../include/allocator_global_heap.h"
#include <new>
#include <cstring>

using allocator_global_heap_t = allocator_global_heap;

allocator_global_heap::allocator_global_heap()
{
    // nothing to do;
}

[[nodiscard]] void *allocator_global_heap::do_allocate_sm(
    size_t size)
{
    std::lock_guard lock(_mutex);

    // allocate extra space to store the allocated size
    size_t total = size + size_t_size;
    void* raw = nullptr;
    try
    {
        raw = ::operator new(total);
    }
    catch (...) //  bad_alloc
    {
        throw;
    }

    *reinterpret_cast<size_t*>(raw) = size;

    return reinterpret_cast<char*>(raw) + size_t_size;
}

void allocator_global_heap::do_deallocate_sm(
    void *at)
{
    if (at == nullptr)
        return;

    std::lock_guard lock(_mutex);

    void* raw = reinterpret_cast<void*>(reinterpret_cast<char*>(at) - size_t_size);
    ::operator delete(raw);
}

allocator_global_heap::~allocator_global_heap()
{
}

allocator_global_heap::allocator_global_heap(const allocator_global_heap &other)
{
    //
}

allocator_global_heap &allocator_global_heap::operator=(const allocator_global_heap &other)
{
    if (this == &other)
        return *this;
    return *this;
}

bool allocator_global_heap::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return dynamic_cast<const allocator_global_heap*>(&other) != nullptr;
}

allocator_global_heap::allocator_global_heap(allocator_global_heap &&other) noexcept
{
}

allocator_global_heap &allocator_global_heap::operator=(allocator_global_heap &&other) noexcept
{
    if (this == &other)
        return *this;
    return *this;
}
