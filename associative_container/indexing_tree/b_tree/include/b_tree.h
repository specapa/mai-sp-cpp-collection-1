#ifndef SYS_PROG_B_TREE_H
#define SYS_PROG_B_TREE_H

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <optional>
#include <stack>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <associative_container.h>
#include <boost/container/static_vector.hpp>
#include <pp_allocator.h>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class B_tree final : private compare
{
    static_assert(t >= 2, "B-tree minimum degree must be at least 2");

public:
    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

    class tree_exception : public std::runtime_error
    {
    public:
        explicit tree_exception(const char* message) : std::runtime_error(message) {}
    };

    class key_not_found : public std::out_of_range
    {
    public:
        explicit key_not_found(const char* message) : std::out_of_range(message) {}
    };

    class invalid_iterator : public tree_exception
    {
    public:
        explicit invalid_iterator(const char* message) : tree_exception(message) {}
    };

    class btree_iterator;
    class btree_reverse_iterator;
    class btree_const_iterator;
    class btree_const_reverse_iterator;

private:
    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;
    static constexpr const size_t overflow_keys_in_node = maximum_keys_in_node + 1;

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;
    inline bool keys_equal(const tkey& lhs, const tkey& rhs) const;

    struct btree_node
    {
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<btree_node*, maximum_keys_in_node + 2> _pointers;

        btree_node() noexcept = default;
    };

    using node_allocator = pp_allocator<btree_node>;
    using mutable_path_type = std::stack<std::pair<btree_node**, size_t>>;
    using const_path_type = std::stack<std::pair<btree_node* const*, size_t>>;

    pp_allocator<value_type> _allocator;
    btree_node* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

    node_allocator make_node_allocator() const noexcept;
    btree_node* create_node();
    void destroy_node(btree_node* node) noexcept;
    void destroy_subtree(btree_node* node) noexcept;
    btree_node* clone_subtree(const btree_node* node);

    static bool is_leaf(const btree_node* node) noexcept;
    size_t lower_index(const btree_node* node, const tkey& key) const;
    size_t upper_index(const btree_node* node, const tkey& key) const;

    btree_node** child_slot(btree_node** node_slot, size_t index) noexcept;
    btree_node* const* child_slot(btree_node* const* node_slot, size_t index) const noexcept;

    mutable_path_type mutable_path_to_lower_bound(const tkey& key, size_t& index);
    const_path_type const_path_to_lower_bound(const tkey& key, size_t& index) const;
    mutable_path_type mutable_path_to_first(size_t& index);
    const_path_type const_path_to_first(size_t& index) const;
    mutable_path_type mutable_path_to_last(size_t& index);
    const_path_type const_path_to_last(size_t& index) const;

    void split_overflow_path(mutable_path_type& path);

    template <typename data_type>
    std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool> insert_impl(data_type&& data);

    template <typename data_type>
    typename B_tree<tkey, tvalue, compare, t>::btree_iterator insert_or_assign_impl(data_type&& data);

    bool erase_from_node(btree_node* node, const tkey& key);
    tree_data_type pop_max(btree_node* node);
    tree_data_type pop_min(btree_node* node);

public:
    explicit B_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());
    explicit B_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit B_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    B_tree(const B_tree& other);
    B_tree(B_tree&& other) noexcept;
    B_tree& operator=(const B_tree& other);
    B_tree& operator=(B_tree&& other) noexcept;
    ~B_tree() noexcept;

    class btree_iterator final
    {
        btree_node** _root;
        mutable_path_type _path;
        size_t _index;

        btree_node* current_node() const noexcept;
        static void descend_first(mutable_path_type& path, btree_node** node_slot);
        static void descend_last(mutable_path_type& path, btree_node** node_slot);

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);
        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_iterator(const mutable_path_type& path = mutable_path_type(), size_t index = 0);

    private:
        btree_iterator(btree_node** root, const mutable_path_type& path, size_t index);
    };

    class btree_const_iterator final
    {
        btree_node* const* _root;
        const_path_type _path;
        size_t _index;

        const btree_node* current_node() const noexcept;
        static void descend_first(const_path_type& path, btree_node* const* node_slot);
        static void descend_last(const_path_type& path, btree_node* const* node_slot);

    public:
        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_iterator;
        friend class btree_const_reverse_iterator;

        btree_const_iterator(const btree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);
        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_iterator(const const_path_type& path = const_path_type(), size_t index = 0);

    private:
        btree_const_iterator(btree_node* const* root, const const_path_type& path, size_t index);
    };

    class btree_reverse_iterator final
    {
        btree_iterator _base;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_reverse_iterator;

        friend class B_tree;
        friend class btree_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        btree_reverse_iterator(const btree_iterator& it) noexcept;
        operator btree_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);
        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_reverse_iterator(const mutable_path_type& path = mutable_path_type(), size_t index = 0);

    private:
        btree_reverse_iterator(btree_node** root, const mutable_path_type& path, size_t index);
    };

    class btree_const_reverse_iterator final
    {
        btree_const_iterator _base;

    public:
        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_reverse_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_iterator;

        btree_const_reverse_iterator(const btree_reverse_iterator& it) noexcept;
        operator btree_const_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);
        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_reverse_iterator(const const_path_type& path = const_path_type(), size_t index = 0);

    private:
        btree_const_reverse_iterator(btree_node* const* root, const const_path_type& path, size_t index);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;
    friend class btree_reverse_iterator;
    friend class btree_const_reverse_iterator;

    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    btree_iterator begin();
    btree_iterator end();

    btree_const_iterator begin() const;
    btree_const_iterator end() const;

    btree_const_iterator cbegin() const;
    btree_const_iterator cend() const;

    btree_reverse_iterator rbegin();
    btree_reverse_iterator rend();

    btree_const_reverse_iterator rbegin() const;
    btree_const_reverse_iterator rend() const;

    btree_const_reverse_iterator crbegin() const;
    btree_const_reverse_iterator crend() const;

    size_t size() const noexcept;
    bool empty() const noexcept;

    btree_iterator find(const tkey& key);
    btree_const_iterator find(const tkey& key) const;

    btree_iterator lower_bound(const tkey& key);
    btree_const_iterator lower_bound(const tkey& key) const;

    btree_iterator upper_bound(const tkey& key);
    btree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    void clear() noexcept;

    std::pair<btree_iterator, bool> insert(const tree_data_type& data);
    std::pair<btree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<btree_iterator, bool> emplace(Args&&... args);

    btree_iterator insert_or_assign(const tree_data_type& data);
    btree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    btree_iterator emplace_or_assign(Args&&... args);

    btree_iterator erase(btree_iterator pos);
    btree_iterator erase(btree_const_iterator pos);

    btree_iterator erase(btree_iterator beg, btree_iterator en);
    btree_iterator erase(btree_const_iterator beg, btree_const_iterator en);

    btree_iterator erase(const tkey& key);
};

template<std::input_iterator iterator,
        comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
B_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>())
        -> B_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>())
        -> B_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_keys(const tkey& lhs, const tkey& rhs) const
{
    return compare::operator()(lhs, rhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::keys_equal(const tkey& lhs, const tkey& rhs) const
{
    return !compare_keys(lhs, rhs) && !compare_keys(rhs, lhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename B_tree<tkey, tvalue, compare, t>::value_type> B_tree<tkey, tvalue, compare, t>::get_allocator() const noexcept
{
    return _allocator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::node_allocator B_tree<tkey, tvalue, compare, t>::make_node_allocator() const noexcept
{
    return node_allocator(_allocator);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node* B_tree<tkey, tvalue, compare, t>::create_node()
{
    return make_node_allocator().template new_object<btree_node>();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::destroy_node(btree_node* node) noexcept
{
    if (node == nullptr) {
        return;
    }

    try {
        make_node_allocator().template delete_object<btree_node>(node);
    } catch (...) {
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::destroy_subtree(btree_node* node) noexcept
{
    if (node == nullptr) {
        return;
    }

    for (btree_node* child : node->_pointers) {
        destroy_subtree(child);
    }
    destroy_node(node);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node* B_tree<tkey, tvalue, compare, t>::clone_subtree(const btree_node* node)
{
    if (node == nullptr) {
        return nullptr;
    }

    btree_node* cloned = create_node();
    try {
        cloned->_keys = node->_keys;
        for (const btree_node* child : node->_pointers) {
            cloned->_pointers.push_back(clone_subtree(child));
        }
    } catch (...) {
        destroy_subtree(cloned);
        throw;
    }
    return cloned;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::is_leaf(const btree_node* node) noexcept
{
    return node == nullptr || node->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::lower_index(const btree_node* node, const tkey& key) const
{
    size_t left = 0;
    size_t right = node->_keys.size();
    while (left < right) {
        const size_t middle = left + (right - left) / 2;
        if (compare_keys(node->_keys[middle].first, key)) {
            left = middle + 1;
        } else {
            right = middle;
        }
    }
    return left;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::upper_index(const btree_node* node, const tkey& key) const
{
    size_t left = 0;
    size_t right = node->_keys.size();
    while (left < right) {
        const size_t middle = left + (right - left) / 2;
        if (!compare_keys(key, node->_keys[middle].first)) {
            left = middle + 1;
        } else {
            right = middle;
        }
    }
    return left;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node** B_tree<tkey, tvalue, compare, t>::child_slot(btree_node** node_slot, size_t index) noexcept
{
    return &((*node_slot)->_pointers[index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node* const* B_tree<tkey, tvalue, compare, t>::child_slot(btree_node* const* node_slot, size_t index) const noexcept
{
    return &((*node_slot)->_pointers[index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::mutable_path_type B_tree<tkey, tvalue, compare, t>::mutable_path_to_lower_bound(const tkey& key, size_t& index)
{
    mutable_path_type path;
    btree_node** slot = &_root;

    while (*slot != nullptr) {
        btree_node* node = *slot;
        const size_t pos = lower_index(node, key);
        path.push({slot, pos});
        if (pos < node->_keys.size() && keys_equal(node->_keys[pos].first, key)) {
            index = pos;
            return path;
        }
        if (is_leaf(node)) {
            index = pos;
            return path;
        }
        slot = child_slot(slot, pos);
    }

    index = 0;
    return path;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::const_path_type B_tree<tkey, tvalue, compare, t>::const_path_to_lower_bound(const tkey& key, size_t& index) const
{
    const_path_type path;
    btree_node* const* slot = &_root;

    while (*slot != nullptr) {
        const btree_node* node = *slot;
        const size_t pos = lower_index(node, key);
        path.push({slot, pos});
        if (pos < node->_keys.size() && keys_equal(node->_keys[pos].first, key)) {
            index = pos;
            return path;
        }
        if (is_leaf(node)) {
            index = pos;
            return path;
        }
        slot = child_slot(slot, pos);
    }

    index = 0;
    return path;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::mutable_path_type B_tree<tkey, tvalue, compare, t>::mutable_path_to_first(size_t& index)
{
    mutable_path_type path;
    btree_node** slot = &_root;
    while (*slot != nullptr) {
        path.push({slot, 0});
        if (is_leaf(*slot)) {
            index = 0;
            return path;
        }
        slot = child_slot(slot, 0);
    }
    index = 0;
    return path;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::const_path_type B_tree<tkey, tvalue, compare, t>::const_path_to_first(size_t& index) const
{
    const_path_type path;
    btree_node* const* slot = &_root;
    while (*slot != nullptr) {
        path.push({slot, 0});
        if (is_leaf(*slot)) {
            index = 0;
            return path;
        }
        slot = child_slot(slot, 0);
    }
    index = 0;
    return path;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::mutable_path_type B_tree<tkey, tvalue, compare, t>::mutable_path_to_last(size_t& index)
{
    mutable_path_type path;
    btree_node** slot = &_root;
    while (*slot != nullptr) {
        const size_t child_index = (*slot)->_keys.size();
        path.push({slot, child_index});
        if (is_leaf(*slot)) {
            index = (*slot)->_keys.size() - 1;
            return path;
        }
        slot = child_slot(slot, child_index);
    }
    index = 0;
    return path;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::const_path_type B_tree<tkey, tvalue, compare, t>::const_path_to_last(size_t& index) const
{
    const_path_type path;
    btree_node* const* slot = &_root;
    while (*slot != nullptr) {
        const size_t child_index = (*slot)->_keys.size();
        path.push({slot, child_index});
        if (is_leaf(*slot)) {
            index = (*slot)->_keys.size() - 1;
            return path;
        }
        slot = child_slot(slot, child_index);
    }
    index = 0;
    return path;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::split_overflow_path(mutable_path_type& path)
{
    while (!path.empty()) {
        btree_node** node_slot = path.top().first;
        btree_node* node = *node_slot;
        if (node->_keys.size() <= maximum_keys_in_node) {
            return;
        }

        const bool leaf = is_leaf(node);
        btree_node* right = create_node();
        tree_data_type promoted = std::move(node->_keys[t]);

        try {
            for (size_t i = t + 1; i < overflow_keys_in_node; ++i) {
                right->_keys.push_back(std::move(node->_keys[i]));
            }
            node->_keys.resize(t);

            if (!leaf) {
                for (size_t i = t + 1; i < node->_pointers.size(); ++i) {
                    right->_pointers.push_back(node->_pointers[i]);
                }
                node->_pointers.resize(t + 1);
            }

            path.pop();
            if (path.empty()) {
                btree_node* new_root = create_node();
                try {
                    new_root->_keys.push_back(std::move(promoted));
                    new_root->_pointers.push_back(node);
                    new_root->_pointers.push_back(right);
                } catch (...) {
                    destroy_node(new_root);
                    throw;
                }
                *node_slot = new_root;
                return;
            }

            btree_node* parent = *path.top().first;
            const size_t parent_index = path.top().second;
            parent->_keys.insert(parent->_keys.begin() + static_cast<ptrdiff_t>(parent_index), std::move(promoted));
            parent->_pointers.insert(parent->_pointers.begin() + static_cast<ptrdiff_t>(parent_index + 1), right);
        } catch (...) {
            destroy_subtree(right);
            throw;
        }
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename data_type>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool> B_tree<tkey, tvalue, compare, t>::insert_impl(data_type&& data)
{
    size_t index = 0;
    mutable_path_type path = mutable_path_to_lower_bound(data.first, index);
    if (!path.empty()) {
        btree_node* node = *path.top().first;
        if (index < node->_keys.size() && keys_equal(node->_keys[index].first, data.first)) {
            return {btree_iterator(&_root, path, index), false};
        }
    }

    if (_root == nullptr) {
        _root = create_node();
        try {
            _root->_keys.push_back(std::forward<data_type>(data));
        } catch (...) {
            destroy_node(_root);
            _root = nullptr;
            throw;
        }
        ++_size;
        size_t first_index = 0;
        return {btree_iterator(&_root, mutable_path_to_first(first_index), first_index), true};
    }

    btree_node* leaf = *path.top().first;
    leaf->_keys.insert(leaf->_keys.begin() + static_cast<ptrdiff_t>(index), std::forward<data_type>(data));
    ++_size;

    split_overflow_path(path);
    return {find(leaf->_keys.size() > index ? leaf->_keys[std::min(index, leaf->_keys.size() - 1)].first : data.first), true};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename data_type>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::insert_or_assign_impl(data_type&& data)
{
    size_t index = 0;
    mutable_path_type path = mutable_path_to_lower_bound(data.first, index);
    if (!path.empty()) {
        btree_node* node = *path.top().first;
        if (index < node->_keys.size() && keys_equal(node->_keys[index].first, data.first)) {
            node->_keys[index].second = std::forward<data_type>(data).second;
            return btree_iterator(&_root, path, index);
        }
    }

    const tkey key = data.first;
    insert_impl(std::forward<data_type>(data));
    return find(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::tree_data_type B_tree<tkey, tvalue, compare, t>::pop_max(btree_node* node)
{
    if (!is_leaf(node)) {
        return pop_max(node->_pointers.back());
    }

    tree_data_type result = std::move(node->_keys.back());
    node->_keys.pop_back();
    return result;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::tree_data_type B_tree<tkey, tvalue, compare, t>::pop_min(btree_node* node)
{
    if (!is_leaf(node)) {
        return pop_min(node->_pointers.front());
    }

    tree_data_type result = std::move(node->_keys.front());
    node->_keys.erase(node->_keys.begin());
    return result;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::erase_from_node(btree_node* node, const tkey& key)
{
    if (node == nullptr) {
        return false;
    }

    const size_t index = lower_index(node, key);
    if (index < node->_keys.size() && keys_equal(node->_keys[index].first, key)) {
        if (is_leaf(node)) {
            node->_keys.erase(node->_keys.begin() + static_cast<ptrdiff_t>(index));
        } else if (!node->_pointers[index]->_keys.empty()) {
            node->_keys[index] = pop_max(node->_pointers[index]);
        } else {
            node->_keys[index] = pop_min(node->_pointers[index + 1]);
        }
        return true;
    }

    if (is_leaf(node)) {
        return false;
    }

    return erase_from_node(node->_pointers[index], key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const compare& cmp, pp_allocator<value_type> alloc)
        : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(pp_allocator<value_type> alloc, const compare& comp)
        : B_tree(comp, alloc)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
B_tree<tkey, tvalue, compare, t>::B_tree(iterator begin, iterator end, const compare& cmp, pp_allocator<value_type> alloc)
        : B_tree(cmp, alloc)
{
    try {
        for (auto it = begin; it != end; ++it) {
            insert(*it);
        }
    } catch (...) {
        clear();
        throw;
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp, pp_allocator<value_type> alloc)
        : B_tree(data.begin(), data.end(), cmp, alloc)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::~B_tree() noexcept
{
    clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const B_tree& other)
        : compare(static_cast<const compare&>(other)),
          _allocator(other._allocator.select_on_container_copy_construction()),
          _root(nullptr),
          _size(other._size)
{
    try {
        _root = clone_subtree(other._root);
    } catch (...) {
        _size = 0;
        throw;
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(const B_tree& other)
{
    if (this == &other) {
        return *this;
    }

    B_tree tmp(other);
    static_cast<compare&>(*this) = static_cast<const compare&>(tmp);
    clear();
    _allocator = tmp._allocator;
    _root = tmp._root;
    _size = tmp._size;
    tmp._root = nullptr;
    tmp._size = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(B_tree&& other) noexcept
        : compare(std::move(static_cast<compare&>(other))),
          _allocator(std::move(other._allocator)),
          _root(other._root),
          _size(other._size)
{
    other._root = nullptr;
    other._size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(B_tree&& other) noexcept
{
    if (this == &other) {
        return *this;
    }

    clear();
    static_cast<compare&>(*this) = std::move(static_cast<compare&>(other));
    _allocator = std::move(other._allocator);
    _root = other._root;
    _size = other._size;
    other._root = nullptr;
    other._size = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node* B_tree<tkey, tvalue, compare, t>::btree_iterator::current_node() const noexcept
{
    return _path.empty() ? nullptr : *_path.top().first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::btree_iterator::descend_first(mutable_path_type& path, btree_node** node_slot)
{
    while (*node_slot != nullptr) {
        path.push({node_slot, 0});
        if ((*node_slot)->_pointers.empty()) {
            return;
        }
        node_slot = &((*node_slot)->_pointers.front());
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::btree_iterator::descend_last(mutable_path_type& path, btree_node** node_slot)
{
    while (*node_slot != nullptr) {
        const size_t child_index = (*node_slot)->_keys.size();
        path.push({node_slot, child_index});
        if ((*node_slot)->_pointers.empty()) {
            return;
        }
        node_slot = &((*node_slot)->_pointers[child_index]);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(const mutable_path_type& path, size_t index)
        : _root(nullptr), _path(path), _index(index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(btree_node** root, const mutable_path_type& path, size_t index)
        : _root(root), _path(path), _index(index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::reference B_tree<tkey, tvalue, compare, t>::btree_iterator::operator*() const noexcept
{
    return reinterpret_cast<reference>(current_node()->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::pointer B_tree<tkey, tvalue, compare, t>::btree_iterator::operator->() const noexcept
{
    return std::addressof(operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator& B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++()
{
    btree_node* node = current_node();
    if (node == nullptr) {
        return *this;
    }

    if (!node->_pointers.empty()) {
        btree_node** slot = &(node->_pointers[_index + 1]);
        descend_first(_path, slot);
        _index = 0;
        return *this;
    }

    if (_index + 1 < node->_keys.size()) {
        ++_index;
        return *this;
    }

    while (!_path.empty()) {
        btree_node** child = _path.top().first;
        _path.pop();
        if (!_path.empty()) {
            btree_node* parent = *_path.top().first;
            const auto child_it = std::find(parent->_pointers.begin(), parent->_pointers.end(), *child);
            const size_t child_index = static_cast<size_t>(std::distance(parent->_pointers.begin(), child_it));
            if (child_it != parent->_pointers.end() && child_index < parent->_keys.size()) {
                _index = child_index;
                return *this;
            }
        }
    }

    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++(int)
{
    self copy(*this);
    ++(*this);
    return copy;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator& B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--()
{
    btree_node* node = current_node();
    if (node == nullptr) {
        if (_root != nullptr && *_root != nullptr) {
            descend_last(_path, _root);
            _index = current_node()->_keys.size() - 1;
        }
        return *this;
    }

    if (!node->_pointers.empty()) {
        btree_node** slot = &(node->_pointers[_index]);
        descend_last(_path, slot);
        _index = current_node()->_keys.size() - 1;
        return *this;
    }

    if (_index > 0) {
        --_index;
        return *this;
    }

    while (!_path.empty()) {
        btree_node** child = _path.top().first;
        _path.pop();
        if (!_path.empty()) {
            btree_node* parent = *_path.top().first;
            const auto child_it = std::find(parent->_pointers.begin(), parent->_pointers.end(), *child);
            const size_t child_index = static_cast<size_t>(std::distance(parent->_pointers.begin(), child_it));
            if (child_it != parent->_pointers.end() && child_index > 0) {
                _index = child_index - 1;
                return *this;
            }
        }
    }

    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--(int)
{
    self copy(*this);
    --(*this);
    return copy;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator==(const self& other) const noexcept
{
    return current_node() == other.current_node() && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::depth() const noexcept
{
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::current_node_keys_count() const noexcept
{
    const btree_node* node = current_node();
    return node == nullptr ? 0 : node->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::is_terminate_node() const noexcept
{
    return B_tree::is_leaf(current_node());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const typename B_tree<tkey, tvalue, compare, t>::btree_node* B_tree<tkey, tvalue, compare, t>::btree_const_iterator::current_node() const noexcept
{
    return _path.empty() ? nullptr : *_path.top().first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::btree_const_iterator::descend_first(const_path_type& path, btree_node* const* node_slot)
{
    while (*node_slot != nullptr) {
        path.push({node_slot, 0});
        if ((*node_slot)->_pointers.empty()) {
            return;
        }
        node_slot = &((*node_slot)->_pointers.front());
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::btree_const_iterator::descend_last(const_path_type& path, btree_node* const* node_slot)
{
    while (*node_slot != nullptr) {
        const size_t child_index = (*node_slot)->_keys.size();
        path.push({node_slot, child_index});
        if ((*node_slot)->_pointers.empty()) {
            return;
        }
        node_slot = &((*node_slot)->_pointers[child_index]);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(const const_path_type& path, size_t index)
        : _root(nullptr), _path(path), _index(index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(btree_node* const* root, const const_path_type& path, size_t index)
        : _root(root), _path(path), _index(index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(const btree_iterator& it) noexcept
        : _root(it._root), _index(it._index)
{
    std::vector<std::pair<btree_node**, size_t>> items;
    auto path = it._path;
    while (!path.empty()) {
        items.push_back(path.top());
        path.pop();
    }
    for (auto rit = items.rbegin(); rit != items.rend(); ++rit) {
        _path.push({rit->first, rit->second});
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::reference B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator*() const noexcept
{
    return reinterpret_cast<reference>(current_node()->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::pointer B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator->() const noexcept
{
    return std::addressof(operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator& B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++()
{
    const btree_node* node = current_node();
    if (node == nullptr) {
        return *this;
    }

    if (!node->_pointers.empty()) {
        btree_node* const* slot = &(node->_pointers[_index + 1]);
        descend_first(_path, slot);
        _index = 0;
        return *this;
    }

    if (_index + 1 < node->_keys.size()) {
        ++_index;
        return *this;
    }

    while (!_path.empty()) {
        const btree_node* child = *_path.top().first;
        _path.pop();
        if (!_path.empty()) {
            const btree_node* parent = *_path.top().first;
            const auto child_it = std::find(parent->_pointers.begin(), parent->_pointers.end(), child);
            const size_t child_index = static_cast<size_t>(std::distance(parent->_pointers.begin(), child_it));
            if (child_it != parent->_pointers.end() && child_index < parent->_keys.size()) {
                _index = child_index;
                return *this;
            }
        }
    }

    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++(int)
{
    self copy(*this);
    ++(*this);
    return copy;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator& B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--()
{
    const btree_node* node = current_node();
    if (node == nullptr) {
        if (_root != nullptr && *_root != nullptr) {
            descend_last(_path, _root);
            _index = current_node()->_keys.size() - 1;
        }
        return *this;
    }

    if (!node->_pointers.empty()) {
        btree_node* const* slot = &(node->_pointers[_index]);
        descend_last(_path, slot);
        _index = current_node()->_keys.size() - 1;
        return *this;
    }

    if (_index > 0) {
        --_index;
        return *this;
    }

    while (!_path.empty()) {
        const btree_node* child = *_path.top().first;
        _path.pop();
        if (!_path.empty()) {
            const btree_node* parent = *_path.top().first;
            const auto child_it = std::find(parent->_pointers.begin(), parent->_pointers.end(), child);
            const size_t child_index = static_cast<size_t>(std::distance(parent->_pointers.begin(), child_it));
            if (child_it != parent->_pointers.end() && child_index > 0) {
                _index = child_index - 1;
                return *this;
            }
        }
    }

    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--(int)
{
    self copy(*this);
    --(*this);
    return copy;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator==(const self& other) const noexcept
{
    return current_node() == other.current_node() && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::depth() const noexcept
{
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::current_node_keys_count() const noexcept
{
    const btree_node* node = current_node();
    return node == nullptr ? 0 : node->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::is_terminate_node() const noexcept
{
    return B_tree::is_leaf(current_node());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(const mutable_path_type& path, size_t index)
        : _base(path, index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(btree_node** root, const mutable_path_type& path, size_t index)
        : _base(root, path, index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(const btree_iterator& it) noexcept
        : _base(it)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator btree_iterator() const noexcept
{
    return _base;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::reference B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator*() const noexcept
{
    return *_base;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::pointer B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator->() const noexcept
{
    return _base.operator->();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator& B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++()
{
    --_base;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++(int)
{
    self copy(*this);
    ++(*this);
    return copy;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator& B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--()
{
    ++_base;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--(int)
{
    self copy(*this);
    --(*this);
    return copy;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator==(const self& other) const noexcept
{
    return _base == other._base;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::depth() const noexcept
{
    return _base.depth();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::current_node_keys_count() const noexcept
{
    return _base.current_node_keys_count();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::is_terminate_node() const noexcept
{
    return _base.is_terminate_node();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::index() const noexcept
{
    return _base.index();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(const const_path_type& path, size_t index)
        : _base(path, index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(btree_node* const* root, const const_path_type& path, size_t index)
        : _base(root, path, index)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(const btree_reverse_iterator& it) noexcept
        : _base(it._base)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator btree_const_iterator() const noexcept
{
    return _base;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::reference B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator*() const noexcept
{
    return *_base;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::pointer B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator->() const noexcept
{
    return _base.operator->();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator& B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++()
{
    --_base;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++(int)
{
    self copy(*this);
    ++(*this);
    return copy;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator& B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--()
{
    ++_base;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--(int)
{
    self copy(*this);
    --(*this);
    return copy;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator==(const self& other) const noexcept
{
    return _base == other._base;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::depth() const noexcept
{
    return _base.depth();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::current_node_keys_count() const noexcept
{
    return _base.current_node_keys_count();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::is_terminate_node() const noexcept
{
    return _base.is_terminate_node();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::index() const noexcept
{
    return _base.index();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto it = find(key);
    if (it == end()) {
        throw key_not_found("B_tree key was not found");
    }
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    auto it = find(key);
    if (it == end()) {
        throw key_not_found("B_tree key was not found");
    }
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    return insert(tree_data_type(key, tvalue())).first->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    return insert(tree_data_type(std::move(key), tvalue())).first->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::begin()
{
    size_t index = 0;
    return btree_iterator(&_root, mutable_path_to_first(index), index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::end()
{
    return btree_iterator(&_root, mutable_path_type(), 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::begin() const
{
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::end() const
{
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cbegin() const
{
    size_t index = 0;
    return btree_const_iterator(&_root, const_path_to_first(index), index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cend() const
{
    return btree_const_iterator(&_root, const_path_type(), 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin()
{
    size_t index = 0;
    return btree_reverse_iterator(&_root, mutable_path_to_last(index), index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend()
{
    return btree_reverse_iterator(&_root, mutable_path_type(), 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin() const
{
    return crbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend() const
{
    return crend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crbegin() const
{
    size_t index = 0;
    return btree_const_reverse_iterator(&_root, const_path_to_last(index), index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crend() const
{
    return btree_const_reverse_iterator(&_root, const_path_type(), 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    size_t index = 0;
    auto path = mutable_path_to_lower_bound(key, index);
    if (path.empty()) {
        return end();
    }
    btree_node* node = *path.top().first;
    if (index < node->_keys.size() && keys_equal(node->_keys[index].first, key)) {
        return btree_iterator(&_root, path, index);
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    size_t index = 0;
    auto path = const_path_to_lower_bound(key, index);
    if (path.empty()) {
        return cend();
    }
    const btree_node* node = *path.top().first;
    if (index < node->_keys.size() && keys_equal(node->_keys[index].first, key)) {
        return btree_const_iterator(&_root, path, index);
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    for (auto it = begin(); it != end(); ++it) {
        if (!compare_keys(it->first, key)) {
            return it;
        }
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    for (auto it = cbegin(); it != cend(); ++it) {
        if (!compare_keys(it->first, key)) {
            return it;
        }
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    for (auto it = begin(); it != end(); ++it) {
        if (!compare_keys(it->first, key)) {
            return it;
        }
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    for (auto it = cbegin(); it != cend(); ++it) {
        if (!compare_keys(it->first, key)) {
            return it;
        }
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    return find(key) != cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    destroy_subtree(_root);
    _root = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool> B_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    return insert_impl(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool> B_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    return insert_impl(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool> B_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    return insert(tree_data_type(std::forward<Args>(args)...));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    return insert_or_assign_impl(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    return insert_or_assign_impl(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    return insert_or_assign(tree_data_type(std::forward<Args>(args)...));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::erase(btree_iterator pos)
{
    if (pos == end()) {
        return end();
    }
    const tkey key = pos->first;
    ++pos;
    erase(key);
    return pos;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator pos)
{
    if (pos == cend()) {
        return end();
    }
    return erase(pos->first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::erase(btree_iterator beg, btree_iterator en)
{
    while (beg != en) {
        beg = erase(beg);
    }
    return beg;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator beg, btree_const_iterator en)
{
    while (beg != en) {
        const tkey key = beg->first;
        ++beg;
        erase(key);
    }
    return lower_bound(en == cend() ? tkey() : en->first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    auto next = find(key);
    if (next != end()) {
        ++next;
    }
    std::optional<tkey> next_key;
    if (next != end()) {
        next_key = next->first;
    }

    if (!erase_from_node(_root, key)) {
        return end();
    }

    --_size;
    if (_root != nullptr && _root->_keys.empty() && !_root->_pointers.empty()) {
        btree_node* old_root = _root;
        _root = _root->_pointers.front();
        old_root->_pointers.clear();
        destroy_node(old_root);
    }
    if (_root != nullptr && _root->_keys.empty() && _root->_pointers.empty()) {
        destroy_node(_root);
        _root = nullptr;
    }
    return next_key.has_value() ? lower_bound(*next_key) : end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool compare_pairs(const typename B_tree<tkey, tvalue, compare, t>::tree_data_type& lhs,
                   const typename B_tree<tkey, tvalue, compare, t>::tree_data_type& rhs)
{
    return compare()(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool compare_keys(const tkey& lhs, const tkey& rhs)
{
    return compare()(lhs, rhs);
}

#endif