// Implementation based on example allocator_sorted_list
#include <cstddef>
#include <new>
#include <algorithm>
#include <utility>
#include <cstring>
#include "../include/allocator_sorted_list.h"


static void** get_first_block_ptr(void* trusted_memory) noexcept
{
    auto byte_ptr = reinterpret_cast<std::byte*>(trusted_memory);
    return reinterpret_cast<void**>(byte_ptr + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex));
}

static void* get_ptr_from_block(void* block_start) noexcept
{
    return *reinterpret_cast<void**>(block_start);
}

static size_t get_block_size(void* block_start) noexcept
{
    auto byte_ptr = reinterpret_cast<std::byte*>(block_start);
    return *reinterpret_cast<size_t*>(byte_ptr + sizeof(void*));
}

static size_t get_overall_size(void* trusted_memory) noexcept
{
    auto byte_ptr = reinterpret_cast<std::byte*>(trusted_memory);
    return *reinterpret_cast<size_t*>(byte_ptr + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode));
}

static std::mutex& get_mutex_ref(void* trusted_memory) noexcept
{
    auto byte_ptr = reinterpret_cast<std::byte*>(trusted_memory);
    return *reinterpret_cast<std::mutex*>(byte_ptr + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
}

allocator_sorted_list::~allocator_sorted_list()
{
    if (_trusted_memory == nullptr)
        return;

    std::pmr::memory_resource* parent = *reinterpret_cast<std::pmr::memory_resource**>(_trusted_memory);
    get_mutex_ref(_trusted_memory).~mutex();

    size_t real_size = get_overall_size(_trusted_memory) + allocator_metadata_size;

    if (parent == nullptr)
    {
        ::operator delete(_trusted_memory);
    }
    else
    {
        parent->deallocate(_trusted_memory, real_size, 1);
    }
}

allocator_sorted_list::allocator_sorted_list(
    allocator_sorted_list &&other) noexcept
{
    _trusted_memory = std::exchange(other._trusted_memory, nullptr);
}

allocator_sorted_list &allocator_sorted_list::operator=(
    allocator_sorted_list &&other) noexcept
{
    std::swap(_trusted_memory, other._trusted_memory);
    return *this;
}

allocator_sorted_list::allocator_sorted_list(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (space_size < block_metadata_size)
    {
        throw std::logic_error("To small space");
    }

    size_t real_size = space_size + allocator_metadata_size;

    _trusted_memory = (parent_allocator == nullptr) ? ::operator new(real_size) : parent_allocator->allocate(real_size, 1);

    auto parent_ptr = reinterpret_cast<std::pmr::memory_resource**>(_trusted_memory);
    *parent_ptr = parent_allocator;

    auto fit_mode_ptr = reinterpret_cast<allocator_with_fit_mode::fit_mode*>(parent_ptr + 1);
    *fit_mode_ptr = allocate_fit_mode;

    auto size_ptr = reinterpret_cast<size_t*>(fit_mode_ptr + 1);
    *size_ptr = space_size;

    auto mutex_ptr = reinterpret_cast<std::mutex*>(size_ptr + 1);
    new (mutex_ptr) std::mutex();

    auto first_block_ptr = reinterpret_cast<void**>(mutex_ptr + 1);
    *first_block_ptr = reinterpret_cast<std::byte*>(_trusted_memory) + allocator_metadata_size;

    auto block_forward_ptr = reinterpret_cast<void**>(first_block_ptr + 1);
    *block_forward_ptr = nullptr;

    auto block_size_ptr = reinterpret_cast<size_t*>(block_forward_ptr + 1);
    *block_size_ptr = space_size - block_metadata_size;
}

[[nodiscard]] void *allocator_sorted_list::do_allocate_sm(
    size_t size)
{
    std::lock_guard lock(get_mutex_ref(_trusted_memory));

    size_t real_size = size;

    void* prev_free = nullptr;

    auto fit = *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<std::pmr::memory_resource**>(_trusted_memory) + 1);

    switch (fit)
    {
        case allocator_with_fit_mode::fit_mode::first_fit:
        {
            void* res_ptr = _trusted_memory;
            for (auto it = free_begin(), sent = free_end(); it != sent; ++it)
            {
                if (it.size() >= real_size)
                {
                    prev_free = res_ptr;
                    break;
                }
                res_ptr = *it;
            }
            break;
        }
        case allocator_with_fit_mode::fit_mode::the_best_fit:
        {
            allocator_sorted_list::sorted_free_iterator res;
            void* res_ptr = nullptr;
            void* prev_ptr = _trusted_memory;

            for (auto it = free_begin(), sent = free_end(); it != sent; ++it)
            {
                if (it.size() >= real_size && (it.size() < res.size() || *res == nullptr))
                {
                    res = it;
                    res_ptr = prev_ptr;
                }

                prev_ptr = *it;
            }

            prev_free = res_ptr;
            break;
        }
        case allocator_with_fit_mode::fit_mode::the_worst_fit:
        {
            allocator_sorted_list::sorted_free_iterator res;
            void* res_ptr = nullptr;
            void* prev_ptr = _trusted_memory;

            for (auto it = free_begin(), sent = free_end(); it != sent; ++it)
            {
                if (it.size() >= real_size && (it.size() > res.size() || *res == nullptr))
                {
                    res = it;
                    res_ptr = prev_ptr;
                }

                prev_ptr = *it;
            }

            prev_free = res_ptr;
            break;
        }
    }

    if (prev_free == nullptr)
    {
        throw std::bad_alloc();
    }

    void* free_block_start = prev_free == _trusted_memory ? *get_first_block_ptr(_trusted_memory) : get_ptr_from_block(prev_free);

    size_t free_block_size = get_block_size(free_block_start);

    bool need_fraq = true;

    if (free_block_size < real_size + block_metadata_size)
    {
        real_size = free_block_size;
        need_fraq = false;
    }

    if (need_fraq)
    {
        void* new_free_block = reinterpret_cast<std::byte*>(free_block_start) + block_metadata_size + real_size;
        (prev_free == _trusted_memory ? *get_first_block_ptr(_trusted_memory) : *reinterpret_cast<void **>(prev_free)) = new_free_block;
        *reinterpret_cast<void **>(new_free_block) = get_ptr_from_block(free_block_start);
        auto size_ptr = reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(new_free_block) + sizeof(void*));
        *size_ptr = free_block_size - real_size - block_metadata_size;
        size_ptr = reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(free_block_start) + sizeof(void*));
        *size_ptr = real_size;
    } else
    {
        (prev_free == _trusted_memory ? *get_first_block_ptr(_trusted_memory) : *reinterpret_cast<void **>(prev_free)) = get_ptr_from_block(free_block_start);
    }
    
    *reinterpret_cast<void**>(free_block_start) = _trusted_memory;

    return reinterpret_cast<std::byte*>(free_block_start) + block_metadata_size;
}

allocator_sorted_list::allocator_sorted_list(const allocator_sorted_list &other)
{
    if (other._trusted_memory == nullptr)
    {
        _trusted_memory = nullptr;
        return;
    }

    std::lock_guard lock(get_mutex_ref(other._trusted_memory));

    std::pmr::memory_resource* parent = *reinterpret_cast<std::pmr::memory_resource**>(other._trusted_memory);
    size_t real_size = get_overall_size(other._trusted_memory) + allocator_metadata_size;

    _trusted_memory = (parent == nullptr) ? ::operator new(real_size) : parent->allocate(real_size, 1);

    std::memcpy(_trusted_memory, other._trusted_memory, real_size);
    new (reinterpret_cast<std::byte*>(_trusted_memory) + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t)) std::mutex();
}

allocator_sorted_list &allocator_sorted_list::operator=(const allocator_sorted_list &other)
{
    if (this == &other)
        return *this;

    allocator_sorted_list tmp(other);
    std::swap(tmp._trusted_memory, _trusted_memory);
    return *this;
}

bool allocator_sorted_list::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return dynamic_cast<const allocator_sorted_list*>(&other) != nullptr;
}

void allocator_sorted_list::do_deallocate_sm(
    void *at)
{
    if (at == nullptr)
        return;

    std::lock_guard lock(get_mutex_ref(_trusted_memory));

    void* block_start = reinterpret_cast<std::byte*>(at) - block_metadata_size;

    if (get_ptr_from_block(block_start) != _trusted_memory)
    {
        throw std::logic_error("Incorrect deallocation object");
    }

    size_t block_size = get_block_size(block_start);

    sorted_free_iterator prev;
    auto it = free_begin();

    for (auto sent = free_end(); it != sent && *it < block_start; ++it)
    {
        prev = it;
    }

    void* prev_free = *prev == nullptr ? _trusted_memory : *prev;

    void* next_free = *it;

    if (prev_free != _trusted_memory && reinterpret_cast<std::byte*>(prev_free) + get_block_size(prev_free) + block_metadata_size == block_start)
    {
        *reinterpret_cast<void**>(prev_free) = next_free;
        *reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(prev_free) + sizeof(void*)) += block_size + block_metadata_size;
        block_start = prev_free;
    } else if (prev_free != _trusted_memory)
    {
        *reinterpret_cast<void**>(prev_free) = block_start;
    } else
    {
        *get_first_block_ptr(_trusted_memory) = block_start;
    }

    if (reinterpret_cast<std::byte*>(block_start) + get_block_size(block_start) + block_metadata_size == next_free)
    {
        *reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(block_start) + sizeof(void*)) +=
                get_block_size(next_free) + block_metadata_size;
        *reinterpret_cast<void**>(block_start) = get_ptr_from_block(next_free);
    } else
    {
        *reinterpret_cast<void**>(block_start) = next_free;
    }
}

inline void allocator_sorted_list::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    std::lock_guard lock(get_mutex_ref(_trusted_memory));
    *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<std::pmr::memory_resource**>(_trusted_memory) + 1) = mode;
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info() const noexcept
{
    std::lock_guard lock(get_mutex_ref(_trusted_memory));
    return get_blocks_info_inner();
}


std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> res;

    std::back_insert_iterator<std::vector<allocator_test_utils::block_info>> inserter(res);

    for(auto it = begin(), sent = end(); it != sent; ++it)
    {
        inserter = {it.size(), it.occupied()};
    }

    return res;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_begin() const noexcept
{
    return {_trusted_memory};
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_end() const noexcept
{
    return {};
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::begin() const noexcept
{
    return {_trusted_memory};
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::end() const noexcept
{
    return {};
}


bool allocator_sorted_list::sorted_free_iterator::operator==(
        const allocator_sorted_list::sorted_free_iterator & other) const noexcept
{
    return _free_ptr == other._free_ptr;
}

bool allocator_sorted_list::sorted_free_iterator::operator!=(
        const allocator_sorted_list::sorted_free_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_sorted_list::sorted_free_iterator &allocator_sorted_list::sorted_free_iterator::operator++() & noexcept
{
    _free_ptr = get_ptr_from_block(_free_ptr);

    return *this;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::sorted_free_iterator::operator++(int n)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

size_t allocator_sorted_list::sorted_free_iterator::size() const noexcept
{
    return _free_ptr == nullptr ? 0 : get_block_size(_free_ptr);
}

void *allocator_sorted_list::sorted_free_iterator::operator*() const noexcept
{
    return _free_ptr;
}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator() : _free_ptr(nullptr){}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator(void *trusted) : _free_ptr(*get_first_block_ptr(trusted)){}

bool allocator_sorted_list::sorted_iterator::operator==(const allocator_sorted_list::sorted_iterator & other) const noexcept
{
    return _current_ptr == other._current_ptr;
}

bool allocator_sorted_list::sorted_iterator::operator!=(const allocator_sorted_list::sorted_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_sorted_list::sorted_iterator &allocator_sorted_list::sorted_iterator::operator++() & noexcept
{
    void* next_free;

    if (_free_ptr != _trusted_memory)
        next_free = get_ptr_from_block(_free_ptr);
    else
        next_free = *get_first_block_ptr(_trusted_memory);

    auto current_size = get_block_size(_current_ptr);

    void* next_block = reinterpret_cast<std::byte*>(_current_ptr) + current_size + block_metadata_size;

    if(next_free == nullptr && next_block >= reinterpret_cast<std::byte*>(_trusted_memory) + get_overall_size(_trusted_memory) + allocator_metadata_size)
    {
        _free_ptr = _current_ptr = nullptr;
    } else if (next_free == next_block)
    {
        _free_ptr = _current_ptr = next_block;
    } else
    {
        _current_ptr = next_block;
    }

    return *this;
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::sorted_iterator::operator++(int n)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

size_t allocator_sorted_list::sorted_iterator::size() const noexcept
{
    if (_current_ptr == nullptr)
        return 0;
    return get_block_size(_current_ptr) + block_metadata_size;
}

void *allocator_sorted_list::sorted_iterator::operator*() const noexcept
{
    return _current_ptr;
}

allocator_sorted_list::sorted_iterator::sorted_iterator() : _current_ptr(nullptr), _trusted_memory(nullptr), _free_ptr(nullptr) {}

allocator_sorted_list::sorted_iterator::sorted_iterator(void *trusted) : _trusted_memory(trusted)
{
    void* start_of_memory = reinterpret_cast<std::byte*>(_trusted_memory) + allocator_metadata_size;

    auto first_free = *get_first_block_ptr(_trusted_memory);

    if (start_of_memory == first_free)
    {
        _current_ptr = _free_ptr = first_free;
    } else
    {
        _current_ptr = start_of_memory;
        _free_ptr = _trusted_memory;
    }
}

bool allocator_sorted_list::sorted_iterator::occupied() const noexcept
{
    return _free_ptr != _current_ptr;
}
