#include <cstddef>
#include <new>
#include <utility>
#include <cstring>
#include "../include/allocator_boundary_tags.h"

size_t allocator_boundary_tags::get_overall_size(void* trusted_memory) noexcept
{
    auto byte_ptr = reinterpret_cast<std::byte*>(trusted_memory);
    return *reinterpret_cast<size_t*>(byte_ptr + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode));
}

std::mutex& allocator_boundary_tags::get_mutex() const noexcept
{
    auto byte_ptr = reinterpret_cast<std::byte*>(_trusted_memory);
    return *reinterpret_cast<std::mutex*>(byte_ptr + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t));
}

allocator_with_fit_mode::fit_mode& allocator_boundary_tags::get_fit_mod() const noexcept
{
    auto parent_ptr = reinterpret_cast<std::pmr::memory_resource**>(_trusted_memory);
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(parent_ptr + 1);
}

void* allocator_boundary_tags::get_first(size_t bytes) const noexcept
{
    void* prev = _trusted_memory;

    for (auto it = begin(), sent = end(); it != sent; ++it)
    {
        if (!it.occupied() && it.size() >= bytes)
            return prev;

        if (it.occupied())
            prev = it.get_ptr();
    }

    return nullptr;
}

void* allocator_boundary_tags::get_best(size_t bytes) const noexcept
{
    void* prev = _trusted_memory;
    void* best_prev = nullptr;
    size_t best_size = static_cast<size_t>(-1);

    for (auto it = begin(), sent = end(); it != sent; ++it)
    {
        if (!it.occupied() && it.size() >= bytes)
        {
            if (best_prev == nullptr || it.size() < best_size)
            {
                best_prev = prev;
                best_size = it.size();
            }
        }

        if (it.occupied())
            prev = it.get_ptr();
    }

    return best_prev;
}

void* allocator_boundary_tags::get_worst(size_t bytes) const noexcept
{
    void* prev = _trusted_memory;
    void* worst_prev = nullptr;
    size_t worst_size = 0;

    for (auto it = begin(), sent = end(); it != sent; ++it)
    {
        if (!it.occupied() && it.size() >= bytes)
        {
            if (worst_prev == nullptr || it.size() > worst_size)
            {
                worst_prev = prev;
                worst_size = it.size();
            }
        }

        if (it.occupied())
            prev = it.get_ptr();
    }

    return worst_prev;
}

allocator_boundary_tags::~allocator_boundary_tags()
{
    get_mutex().~mutex();
    std::pmr::memory_resource* parent = *reinterpret_cast<std::pmr::memory_resource**>(_trusted_memory);
    size_t real_size = get_overall_size(_trusted_memory) + allocator_metadata_size;

    if (parent == nullptr)
        ::operator delete(_trusted_memory);
    else
        parent->deallocate(_trusted_memory, real_size, 1);
}

allocator_boundary_tags::allocator_boundary_tags(
    allocator_boundary_tags &&other) noexcept
{
    _trusted_memory = std::exchange(other._trusted_memory, nullptr);
}

allocator_boundary_tags &allocator_boundary_tags::operator=(
    allocator_boundary_tags &&other) noexcept
{
    if (this != &other)
        std::swap(_trusted_memory, other._trusted_memory);
    return *this;
}


allocator_boundary_tags::allocator_boundary_tags(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (space_size < occupied_block_metadata_size)
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
    *first_block_ptr = nullptr;
}

[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(
    size_t value_size)
{
    std::lock_guard lock(get_mutex());

    size_t real_size = value_size + occupied_block_metadata_size;

    void* prev_occupied = nullptr;

    switch (get_fit_mod())
    {
        case allocator_with_fit_mode::fit_mode::first_fit:
            prev_occupied = get_first(real_size);
            break;
        case allocator_with_fit_mode::fit_mode::the_best_fit:
            prev_occupied = get_best(real_size);
            break;
        case allocator_with_fit_mode::fit_mode::the_worst_fit:
            prev_occupied = get_worst(real_size);
            break;
    }

    if (prev_occupied == nullptr)
    {
        throw std::bad_alloc();
    }

    size_t free_block_size = get_next_free_size(prev_occupied, _trusted_memory);

    if (free_block_size < real_size + occupied_block_metadata_size)
    {
        real_size = free_block_size;
    }

    void* free_block_start;

    if (prev_occupied == _trusted_memory)
    {
        free_block_start = reinterpret_cast<std::byte*>(_trusted_memory) + allocator_metadata_size;
    } else
    {
        free_block_start = reinterpret_cast<std::byte*>(prev_occupied) + get_occupied_size(prev_occupied) + occupied_block_metadata_size;
    }

    auto size_ptr = reinterpret_cast<size_t*>(free_block_start);
    *size_ptr = real_size - occupied_block_metadata_size;

    auto back_ptr = reinterpret_cast<void**>(size_ptr + 1);
    *back_ptr = prev_occupied;

    auto forward_ptr = reinterpret_cast<void**>(back_ptr + 1);
    *forward_ptr = prev_occupied == _trusted_memory ? *get_first_block_ptr(_trusted_memory) : get_next_from_occupied(prev_occupied);

    auto parent_ptr = reinterpret_cast<void**>(forward_ptr + 1);
    *parent_ptr = _trusted_memory;

    void* next_block = prev_occupied == _trusted_memory ? *get_first_block_ptr(_trusted_memory) : get_next_from_occupied(prev_occupied);

    if (next_block != nullptr)
    {
        auto byte_ptr = reinterpret_cast<std::byte*>(next_block);
        byte_ptr += sizeof(size_t);
        *reinterpret_cast<void**>(byte_ptr) = free_block_start;
    }

    if (prev_occupied == _trusted_memory)
    {
        *get_first_block_ptr(_trusted_memory) = free_block_start;
    } else
    {
        auto byte_ptr = reinterpret_cast<std::byte*>(prev_occupied);
        byte_ptr += sizeof(size_t) + sizeof(void*);
        *reinterpret_cast<void**>(byte_ptr) = free_block_start;
    }

    return reinterpret_cast<std::byte*>(free_block_start) + occupied_block_metadata_size;
}

void allocator_boundary_tags::do_deallocate_sm(
    void *at)
{
    if (at == nullptr)
        return;

    std::lock_guard lock(get_mutex());

    void* block_start = reinterpret_cast<std::byte*>(at) - occupied_block_metadata_size;

    if (get_parent_from_occupied(block_start) != _trusted_memory)
    {
        throw std::logic_error("Incorrect deallocation object");
    }

    size_t block_size = get_occupied_size(block_start);

    void* prev_block = get_previous_from_occupied(block_start);
    void* next_block = get_next_from_occupied(block_start);

    if (prev_block == _trusted_memory)
    {
        *get_first_block_ptr(_trusted_memory) = next_block;
    } else
    {
        auto byte_ptr = reinterpret_cast<std::byte*>(prev_block) + sizeof(size_t) + sizeof(void*);
        *reinterpret_cast<void**>(byte_ptr) = next_block;
    }

    if (next_block != nullptr)
    {
        auto byte_ptr = reinterpret_cast<std::byte*>(next_block) + sizeof(size_t);
        *reinterpret_cast<void**>(byte_ptr) = prev_block;
    }
}

inline void allocator_boundary_tags::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    std::lock_guard lock(get_mutex());
    get_fit_mod() = mode;
}


std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const
{
    std::lock_guard lock(get_mutex());
    return get_blocks_info_inner();
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::begin() const noexcept
{
    return {_trusted_memory};
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::end() const noexcept
{
    return {};
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> res;
    std::back_insert_iterator<std::vector<allocator_test_utils::block_info>> inserter(res);

    for(auto it = begin(), sent = end(); it != sent; ++it)
    {
        inserter = {it.size(), it.occupied()};
    }

    return res;
}

allocator_boundary_tags::allocator_boundary_tags(const allocator_boundary_tags &other)
{
    if (other._trusted_memory == nullptr)
    {
        _trusted_memory = nullptr;
        return;
    }

    std::lock_guard lock(other.get_mutex());
    std::pmr::memory_resource* parent = *reinterpret_cast<std::pmr::memory_resource**>(other._trusted_memory);
    size_t real_size = get_overall_size(other._trusted_memory) + allocator_metadata_size;

    _trusted_memory = (parent == nullptr) ? ::operator new(real_size) : parent->allocate(real_size, 1);

    std::memcpy(_trusted_memory, other._trusted_memory, real_size);
    new (reinterpret_cast<std::byte*>(_trusted_memory) + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t)) std::mutex();
}

allocator_boundary_tags &allocator_boundary_tags::operator=(const allocator_boundary_tags &other)
{
    if (this == &other)
        return *this;

    allocator_boundary_tags tmp(other);
    std::swap(tmp._trusted_memory, _trusted_memory);
    return *this;
}

bool allocator_boundary_tags::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return dynamic_cast<const allocator_boundary_tags*>(&other) != nullptr;
}

size_t allocator_boundary_tags::get_occupied_size(void *block_start) noexcept
{
    return *reinterpret_cast<size_t*>(block_start);
}

void *allocator_boundary_tags::get_previous_from_occupied(void *block_start) noexcept
{
    auto byte_ptr = reinterpret_cast<std::byte*>(block_start);
    return *reinterpret_cast<void**>(byte_ptr + sizeof(size_t));
}

void *allocator_boundary_tags::get_next_from_occupied(void *block_start) noexcept
{
    auto byte_ptr = reinterpret_cast<std::byte*>(block_start);
    return *reinterpret_cast<void**>(byte_ptr + sizeof(size_t) + sizeof(void*));
}

void *allocator_boundary_tags::get_parent_from_occupied(void *block_start) noexcept
{
    auto byte_ptr = reinterpret_cast<std::byte*>(block_start);
    return *reinterpret_cast<void**>(byte_ptr + sizeof(size_t) + sizeof(void*) + sizeof(void*));
}

size_t allocator_boundary_tags::get_next_free_size(void *occupied_block_start, void* trusted_memory) noexcept
{
    if (occupied_block_start == trusted_memory)
    {
        return *get_first_block_ptr(trusted_memory) == nullptr ? get_overall_size(trusted_memory)
                            : reinterpret_cast<std::byte*>(*get_first_block_ptr(trusted_memory)) - reinterpret_cast<std::byte*>(trusted_memory) - allocator_metadata_size;
    } else
    {
        return get_next_from_occupied(occupied_block_start) == nullptr ? (reinterpret_cast<std::byte*>(trusted_memory) + allocator_metadata_size +
                get_overall_size(trusted_memory)) - (reinterpret_cast<std::byte*>(occupied_block_start) +
                get_occupied_size(occupied_block_start) + occupied_block_metadata_size) :
                reinterpret_cast<std::byte*>(get_next_from_occupied(occupied_block_start)) -
                (reinterpret_cast<std::byte*>(occupied_block_start) + get_occupied_size(occupied_block_start) + occupied_block_metadata_size);
    }
}

void **allocator_boundary_tags::get_first_block_ptr(void* trusted_memory) noexcept
{
    auto byte_ptr = reinterpret_cast<std::byte*>(trusted_memory);
    return reinterpret_cast<void**>(byte_ptr + sizeof(std::pmr::memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) + sizeof(std::mutex));
}

size_t allocator_boundary_tags::get_free_size() const noexcept
{
    size_t accum = 0;

    for (auto it = begin(), sent = end(); it != sent; ++it)
    {
        if (!it.occupied())
        {
            accum += it.size();
        }
    }
    return accum;
}

bool allocator_boundary_tags::boundary_iterator::operator==(
        const allocator_boundary_tags::boundary_iterator &other) const noexcept
{
    return _occupied_ptr == other._occupied_ptr && ((_occupied == other._occupied) || _occupied_ptr == nullptr || other._occupied_ptr == nullptr);
}

bool allocator_boundary_tags::boundary_iterator::operator!=(
        const allocator_boundary_tags::boundary_iterator & other) const noexcept
{
    return !(*this == other);
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator++() & noexcept
{
    if (!_occupied)
    {
        _occupied = true;
        _occupied_ptr = _trusted_memory == _occupied_ptr ? *get_first_block_ptr(_trusted_memory) : get_next_from_occupied(_occupied_ptr);
    } else
    {
        size_t current_size = get_occupied_size(_occupied_ptr);
        void* next_block = get_next_from_occupied(_occupied_ptr);

        if (next_block == reinterpret_cast<std::byte*>(_occupied_ptr) + current_size + occupied_block_metadata_size ||
                (next_block == nullptr && reinterpret_cast<std::byte*>(_occupied_ptr) + current_size + occupied_block_metadata_size ==
                                                  reinterpret_cast<std::byte*>(_trusted_memory) + get_overall_size(_trusted_memory) + allocator_metadata_size))
            _occupied_ptr = next_block;
        else
            _occupied = false;
    }

    return *this;
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator--() & noexcept
{
    if (!_occupied)
    {
        _occupied = true;
    } else
    {
        void* prev_block = get_previous_from_occupied(_occupied_ptr);

        if(prev_block != _trusted_memory)
        {
            auto size = get_occupied_size(prev_block);

            _occupied = reinterpret_cast<std::byte*>(prev_block) + size + occupied_block_metadata_size == _occupied_ptr;

            _occupied_ptr = prev_block;
        }
    }

    return *this;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator++(int n)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator--(int n)
{
    auto tmp = *this;
    --(*this);
    return tmp;
}

size_t allocator_boundary_tags::boundary_iterator::size() const noexcept
{
    if (_occupied_ptr == nullptr)
        return 0;
    return _occupied ? get_occupied_size(_occupied_ptr) + occupied_block_metadata_size : get_next_free_size(_occupied_ptr, _trusted_memory);
}

bool allocator_boundary_tags::boundary_iterator::occupied() const noexcept
{
    return _occupied;
}

void* allocator_boundary_tags::boundary_iterator::operator*() const noexcept
{
    return _occupied ? _occupied_ptr : reinterpret_cast<std::byte*>(_occupied_ptr) + get_occupied_size(_occupied_ptr) + occupied_block_metadata_size;
}

allocator_boundary_tags::boundary_iterator::boundary_iterator() : _trusted_memory(nullptr), _occupied_ptr(nullptr), _occupied(false){}

allocator_boundary_tags::boundary_iterator::boundary_iterator(void *trusted) : _trusted_memory(trusted), _occupied_ptr(trusted), _occupied(true)
{
    _occupied = *get_first_block_ptr(_trusted_memory) == (reinterpret_cast<std::byte *>(_trusted_memory) + allocator_metadata_size);
    if (_occupied)
        _occupied_ptr = *get_first_block_ptr(_trusted_memory);
}

void *allocator_boundary_tags::boundary_iterator::get_ptr() const noexcept
{
    return _occupied_ptr;
}
