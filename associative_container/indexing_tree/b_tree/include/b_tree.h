#ifndef SYS_PROG_B_TREE_H
#define SYS_PROG_B_TREE_H

#include <iterator>
#include <utility>
#include <boost/container/static_vector.hpp>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <not_implemented.h>
#include <optional>
#include <functional>
#include <initializer_list>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class B_tree final : private compare // EBCO
{
public:

    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

private:

    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;

    // endregion comparators declaration


    struct btree_node
    {
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<btree_node*, maximum_keys_in_node + 2> _pointers;
        btree_node() noexcept : _keys(), _pointers() {}
    };

    pp_allocator<value_type> _allocator;
    btree_node* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept { return _allocator; }

    // insertion helpers
    void split_child(btree_node* parent, size_t index);
    void insert_nonfull(btree_node* node, tree_data_type_const data);
    btree_node* allocate_node();
    void deallocate_node(btree_node* node) noexcept;

public:

    // region constructors declaration

    explicit B_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit B_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit B_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    B_tree(const B_tree& other);

    B_tree(B_tree&& other) noexcept;

    B_tree& operator=(const B_tree& other);

    B_tree& operator=(B_tree&& other) noexcept;

    ~B_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class btree_iterator;
    class btree_reverse_iterator;
    class btree_const_iterator;
    class btree_const_reverse_iterator;

    class btree_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:
        using value_type = tree_data_type;
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

        explicit btree_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);

    };

    class btree_const_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type;
        using reference = const tree_data_type&;
        using pointer = const tree_data_type*;
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

        explicit btree_const_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    class btree_reverse_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type;
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

        explicit btree_reverse_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);
    };

    class btree_const_reverse_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type;
        using reference = const tree_data_type&;
        using pointer = const tree_data_type*;
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

        explicit btree_const_reverse_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;
    friend class btree_reverse_iterator;
    friend class btree_const_reverse_iterator;

    // endregion iterators declaration

    // region element access declaration

    /*
     * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
     */
    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    /*
     * If key not exists, makes default initialization of value
     */
    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    // endregion element access declaration
    // region iterator begins declaration

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

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;
    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    btree_iterator find(const tkey& key);
    btree_const_iterator find(const tkey& key) const;

    btree_iterator lower_bound(const tkey& key);
    btree_const_iterator lower_bound(const tkey& key) const;

    btree_iterator upper_bound(const tkey& key);
    btree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<btree_iterator, bool> insert(const tree_data_type& data);
    std::pair<btree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<btree_iterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    btree_iterator insert_or_assign(const tree_data_type& data);
    btree_iterator insert_or_assign(tree_data_type&& data);
    
    void debug_print_tree() const;

    template <typename ...Args>
    btree_iterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    btree_iterator erase(btree_iterator pos);
    btree_iterator erase(btree_const_iterator pos);

    btree_iterator erase(btree_iterator beg, btree_iterator en);
    btree_iterator erase(btree_const_iterator beg, btree_const_iterator en);


    btree_iterator erase(const tkey& key);

    // endregion modifiers declaration
};

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
B_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_pairs(const B_tree::tree_data_type &lhs,
                                                     const B_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::split_child(btree_node* parent, size_t index)
{
    btree_node* child = parent->_pointers[index];
    btree_node* sibling = allocate_node();
    
    // For leaves with t >= 3: use mid=t (left biased)
    // For leaves with t == 2 or internal nodes: use mid=t-1 (balanced)
    size_t mid = (child->_pointers.empty() && t >= 3) ? t : t - 1;

    // remember middle key
    tree_data_type_const middle = child->_keys[mid];

    // move keys after middle to sibling
    for (size_t j = mid + 1; j < child->_keys.size(); ++j)
        sibling->_keys.emplace_back(std::move(child->_keys[j]));

    // move pointers if any
    if (!child->_pointers.empty())
    {
        for (size_t j = mid + 1; j < child->_pointers.size(); ++j)
            sibling->_pointers.emplace_back(child->_pointers[j]);
    }

    // shrink child
    while (child->_keys.size() > mid) child->_keys.pop_back();
    if (!child->_pointers.empty())
    {
        while (child->_pointers.size() > mid + 1) child->_pointers.pop_back();
    }

    // insert middle into parent
    parent->_keys.insert(parent->_keys.begin() + index, middle);
    parent->_pointers.insert(parent->_pointers.begin() + index + 1, sibling);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::insert_nonfull(btree_node* node, tree_data_type_const data)
{
    size_t i = 0;
    while (i < node->_keys.size() && compare_keys(node->_keys[i].first, data.first)) ++i;

    if (node->_pointers.empty())
    {
        node->_keys.insert(node->_keys.begin() + i, data);
        return;
    }
    else
    {
        btree_node* child = node->_pointers[i];
        if (child->_keys.size() == maximum_keys_in_node)
        {
            split_child(node, i);
            if (compare_keys(node->_keys[i].first, data.first)) ++i;
        }
        insert_nonfull(node->_pointers[i], data);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node* B_tree<tkey, tvalue, compare, t>::allocate_node()
{
    return _allocator.template new_object<btree_node>();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::deallocate_node(btree_node* node) noexcept
{
    if (node == nullptr) return;
    _allocator.template delete_object<btree_node>(node);
}

// get_allocator implemented inline above

// region constructors implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        const compare& cmp,
        pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        pp_allocator<value_type> alloc,
        const compare& comp)
    : compare(comp), _allocator(alloc), _root(nullptr), _size(0)
{
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
B_tree<tkey, tvalue, compare, t>::B_tree(
        iterator begin,
        iterator end,
        const compare& cmp,
        pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (; begin != end; ++begin)
    {
        emplace(begin->first, begin->second);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        std::initializer_list<std::pair<tkey, tvalue>> data,
        const compare& cmp,
        pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (auto &it: data)
        emplace(it.first, it.second);
}

// endregion constructors implementation

// region five implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::~B_tree() noexcept
{
    clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const B_tree& other)
{
    _allocator = other._allocator;
    _root = nullptr;
    _size = 0;
    if (other._root)
    {
        _root = copy_subtree(nullptr, other._root);
        _size = other._size;
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(const B_tree& other)
{
    if (this == &other) return *this;
    clear();
    _allocator = other._allocator;
    if (other._root)
    {
        _root = copy_subtree(nullptr, other._root);
        _size = other._size;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(B_tree&& other) noexcept
{
    _allocator = other._allocator;
    _root = other._root;
    _size = other._size;
    other._root = nullptr;
    other._size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(B_tree&& other) noexcept
{
    if (this != &other)
    {
        clear();
        _allocator = other._allocator;
        _root = other._root;
        _size = other._size;
        other._root = nullptr;
        other._size = 0;
    }
    return *this;
}

// endregion five implementation

// region iterators implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index)
{
    _path = path;
    _index = index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator*() const noexcept
{
    return (*_path.top().first)->_keys[_index];
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator->() const noexcept
{
    return std::addressof((*_path.top().first)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++()
{
    if (_path.empty()) return *this;

    btree_node* node = *(_path.top().first);

    // if node has children, go to leftmost of right child
    if (!node->_pointers.empty())
    {
        btree_node** child_ptr = &node->_pointers[_index + 1];
        _path.push(std::make_pair(child_ptr, _index + 1));
        // descend to leftmost
        while (!(*_path.top().first)->_pointers.empty())
        {
            btree_node* cur = *(_path.top().first);
            btree_node** next_ptr = &cur->_pointers[0];
            _path.push(std::make_pair(next_ptr, 0));
        }
        _index = 0;
        return *this;
    }

    // leaf: move to next key or ascend
    if (_index + 1 < node->_keys.size())
    {
        ++_index;
        return *this;
    }

    // ascend
    while (!_path.empty())
    {
        auto top = _path.top();
        _path.pop();
        if (_path.empty())
            break;
        btree_node* parent = *(_path.top().first);
        size_t child_index = top.second;
        if (child_index < parent->_keys.size())
        {
            _index = child_index;
            return *this;
        }
    }

    // reached end
    _path = std::stack<std::pair<btree_node**, size_t>>();
    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++(int)
{
    self tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--()
{
    if (_path.empty()) return *this;

    btree_node* node = *(_path.top().first);

    // if node has children, go to rightmost of left child
    if (!node->_pointers.empty())
    {
        btree_node** child_ptr = &node->_pointers[_index];
        _path.push(std::make_pair(child_ptr, _index));
        // descend to rightmost
        while (!(*_path.top().first)->_pointers.empty())
        {
            btree_node* cur = *(_path.top().first);
            size_t last = cur->_pointers.size() - 1;
            btree_node** next_ptr = &cur->_pointers[last];
            _path.push(std::make_pair(next_ptr, last));
        }
        _index = (*_path.top().first)->_keys.size() - 1;
        return *this;
    }

    // leaf: move to previous key or ascend
    if (_index > 0)
    {
        --_index;
        return *this;
    }

    // ascend
    while (!_path.empty())
    {
        auto top = _path.top();
        _path.pop();
        if (_path.empty())
            break;
        btree_node* parent = *(_path.top().first);
        size_t child_index = top.second;
        if (child_index > 0)
        {
            _index = child_index - 1;
            return *this;
        }
    }

    // reached begin
    _path = std::stack<std::pair<btree_node**, size_t>>();
    _index = 0;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--(int)
{
    self tmp = *this;
    --(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator==(const self& other) const noexcept
{
    if (_index != other._index) return false;
    // compare stacks by unwinding to vectors
    std::stack<std::pair<btree_node**, size_t>> a = _path;
    std::stack<std::pair<btree_node**, size_t>> b = other._path;
    while (!a.empty() && !b.empty())
    {
        if (a.top().first != b.top().first || a.top().second != b.top().second) return false;
        a.pop(); b.pop();
    }
    return a.empty() && b.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::depth() const noexcept
{
    if (_path.empty()) return 0;
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) return 0;
    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::is_terminate_node() const noexcept
{
    if (_path.empty()) return true;
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index)
{
    _path = path;
    _index = index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const btree_iterator& it) noexcept
{
    // convert stack<pair<btree_node**, size_t>> to stack<pair<btree_node* const*, size_t>>
    std::stack<std::pair<btree_node**, size_t>> tmp = it._path;
    std::vector<std::pair<btree_node* const*, size_t>> v;
    while (!tmp.empty())
    {
        auto p = tmp.top(); tmp.pop();
        v.emplace_back(reinterpret_cast<btree_node* const*>(p.first), p.second);
    }
    // v holds reversed order, push back to correct stack order
    for (auto itv = v.rbegin(); itv != v.rend(); ++itv)
        _path.push(*itv);
    _index = it._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator*() const noexcept
{
    auto &p = (*_path.top().first)->_keys[_index];
    std::cerr << "ITER: key=" << p.first << " depth=" << depth() << " index=" << index() 
              << " node_keys=[";
    for (auto &k : (*_path.top().first)->_keys) std::cerr << k.first << ",";
    std::cerr << "]\n";
    return p;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator->() const noexcept
{
    return std::addressof((*_path.top().first)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++()
{
    // convert to mutable iterator, increment, then convert back
    std::stack<std::pair<btree_node**, size_t>> tmp;
    // build mutable stack
    std::vector<std::pair<btree_node**, size_t>> vec;
    std::stack<std::pair<btree_node* const*, size_t>> copy = _path;
    while (!copy.empty()) { auto p = copy.top(); copy.pop(); vec.emplace_back(const_cast<btree_node**>(p.first), p.second); }
    for (auto it = vec.rbegin(); it != vec.rend(); ++it) tmp.push(*it);
    btree_iterator mut(tmp, _index);
    ++mut;
    // convert back
    _path = std::stack<std::pair<btree_node* const*, size_t>>();
    std::vector<std::pair<btree_node* const*, size_t>> v2;
    while (!mut._path.empty()) { v2.emplace_back(reinterpret_cast<btree_node* const*>(mut._path.top().first), mut._path.top().second); mut._path.pop(); }
    for (auto it = v2.rbegin(); it != v2.rend(); ++it) _path.push(*it);
    _index = mut._index;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++(int)
{
    self tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--()
{
    // convert to mutable iterator, decrement, then convert back
    std::stack<std::pair<btree_node**, size_t>> tmp;
    std::vector<std::pair<btree_node**, size_t>> vec;
    std::stack<std::pair<btree_node* const*, size_t>> copy = _path;
    while (!copy.empty()) { auto p = copy.top(); copy.pop(); vec.emplace_back(const_cast<btree_node**>(p.first), p.second); }
    for (auto it = vec.rbegin(); it != vec.rend(); ++it) tmp.push(*it);
    btree_iterator mut(tmp, _index);
    --mut;
    // convert back
    _path = std::stack<std::pair<btree_node* const*, size_t>>();
    std::vector<std::pair<btree_node* const*, size_t>> v2;
    while (!mut._path.empty()) { v2.emplace_back(reinterpret_cast<btree_node* const*>(mut._path.top().first), mut._path.top().second); mut._path.pop(); }
    for (auto it = v2.rbegin(); it != v2.rend(); ++it) _path.push(*it);
    _index = mut._index;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--(int)
{
    self tmp = *this;
    --(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator==(const self& other) const noexcept
{
    if (_index != other._index) return false;
    auto a = _path; auto b = other._path;
    while (!a.empty() && !b.empty())
    {
        if (a.top().first != b.top().first || a.top().second != b.top().second) return false;
        a.pop(); b.pop();
    }
    return a.empty() && b.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::depth() const noexcept
{
    if (_path.empty()) return 0;
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) return 0;
    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::is_terminate_node() const noexcept
{
    if (_path.empty()) return true;
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(\n"
                          "const std::stack<std::pair<btree_node**, size_t>>& path, size_t index)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const btree_iterator& it) noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(\n"
                          "const btree_iterator& it) noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_iterator() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator btree_iterator() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator*() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::reference\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator*() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator->() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::pointer\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator->() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++()
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++()", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++(int)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++(int)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--()
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--()", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--(int)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--(int)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator==(const self& other) const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator==(const self& other) const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator!=(const self& other) const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator!=(const self& other) const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::depth() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::depth() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::current_node_keys_count() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::current_node_keys_count() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::is_terminate_node() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::is_terminate_node() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::index() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::index() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(\n"
                          "const std::stack<std::pair<const btree_node**, size_t>>& path, size_t index)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const btree_reverse_iterator& it) noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(\n"
                          "const btree_reverse_iterator& it) noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_const_iterator() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator btree_const_iterator() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator*() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::reference\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator*() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator->() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::pointer\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator->() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++()
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++()", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++(int)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++(int)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--()
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--()", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--(int)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator\n"
                          "B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--(int)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator==(const self& other) const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator==(const self& other) const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator!=(const self& other) const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator!=(const self& other) const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::depth() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::depth() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::current_node_keys_count() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::current_node_keys_count() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::is_terminate_node() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::is_terminate_node() const noexcept", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::index() const noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::index() const noexcept", "your code should be here...");
}

// endregion iterators implementation

// region element access implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto it = find(key);
    if (it == end()) throw std::out_of_range("key not found");
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    auto it = find(key);
    if (it == end()) throw std::out_of_range("key not found");
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    auto it = lower_bound(key);
    if (it == end() || compare_keys(key, it->first) || compare_keys(it->first, key))
    {
        emplace(key, tvalue());
    }
    return find(key)->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    auto it = lower_bound(key);
    if (it == end() || compare_keys(key, it->first) || compare_keys(it->first, key))
    {
        emplace(std::move(key), tvalue());
    }
    return find(key)->second;
}

// endregion element access implementation

// region iterator begins implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::begin()
{
    if (_root == nullptr) return btree_iterator();
    std::vector<std::pair<btree_node**, size_t>> path;
    btree_node** cur_ptr = &_root;
    btree_node* cur = *cur_ptr;
    path.emplace_back(cur_ptr, 0);
    while (cur && !cur->_pointers.empty())
    {
        cur_ptr = &cur->_pointers[0];
        cur = *cur_ptr;
        path.emplace_back(cur_ptr, 0);
    }
    std::stack<std::pair<btree_node**, size_t>> st;
    for (auto &p: path) st.push(p);
    return btree_iterator(st, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::end()
{
    return btree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::begin() const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::end() const
{
    return btree_const_iterator(btree_iterator());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cbegin() const
{
    return begin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cend() const
{
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin()
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin()", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend()
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend()", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin() const
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin() const", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend() const
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend() const", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crbegin() const
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crbegin() const", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crend() const
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crend() const", "your code should be here...");
}

// endregion iterator begins implementation

// region lookup implementation

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
    if (_root == nullptr) return end();
    std::vector<std::pair<btree_node**, size_t>> path;
    btree_node** cur_ptr = &_root;
    btree_node* cur = *cur_ptr;
    path.emplace_back(cur_ptr, 0);

    while (cur)
    {
        size_t i = 0;
        while (i < cur->_keys.size() && compare_keys(cur->_keys[i].first, key)) ++i;
        if (i < cur->_keys.size() && !compare_keys(key, cur->_keys[i].first) && !compare_keys(cur->_keys[i].first, key))
        {
            // found
            std::stack<std::pair<btree_node**, size_t>> st;
            for (auto &p: path) st.push(p);
            return btree_iterator(st, i);
        }
        if (cur->_pointers.empty()) break;
        // descend
        cur_ptr = &cur->_pointers[i];
        cur = *cur_ptr;
        path.emplace_back(cur_ptr, i);
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    auto it = const_cast<B_tree*>(this)->find(key);
    return btree_const_iterator(it);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    if (_root == nullptr) return end();
    std::vector<std::pair<btree_node**, size_t>> path;
    btree_node** cur_ptr = &_root;
    btree_node* cur = *cur_ptr;
    path.emplace_back(cur_ptr, 0);

    while (cur)
    {
        size_t i = 0;
        while (i < cur->_keys.size() && compare_keys(cur->_keys[i].first, key)) ++i;
        if (cur->_pointers.empty())
        {
            if (i < cur->_keys.size())
            {
                std::stack<std::pair<btree_node**, size_t>> st;
                for (auto &p: path) st.push(p);
                return btree_iterator(st, i);
            }
            break;
        }
        cur_ptr = &cur->_pointers[i];
        cur = *cur_ptr;
        path.emplace_back(cur_ptr, i);
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    return const_cast<B_tree*>(this)->lower_bound(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    return lower_bound(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    return const_cast<B_tree*>(this)->upper_bound(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    return find(key) != end();
}

// endregion lookup implementation

// region modifiers implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    if (_root == nullptr) return;
    std::stack<btree_node*> st;
    st.push(_root);
    while (!st.empty()) {
        btree_node* node = st.top(); st.pop();
        for (auto ptr : node->_pointers) if (ptr) st.push(ptr);
        deallocate_node(node);
    }
    _root = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    return emplace(data.first, data.second);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    return emplace(std::move(data.first), std::move(data.second));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    // construct a temporary pair and insert
    tree_data_type tmp(std::forward<Args>(args)...);
    // if empty tree
    if (_root == nullptr)
    {
        btree_node* n = allocate_node();
        n->_keys.emplace_back(std::make_pair(tmp.first, std::move(tmp.second)));
        // leaf: pointers empty
        _root = n;
        ++_size;
        return std::make_pair(begin(), true);
    }

    // if root full, split
    if (_root->_keys.size() == maximum_keys_in_node)
    {
        btree_node* s = allocate_node();
        s->_pointers.emplace_back(_root);
        _root = s;
        split_child(s, 0);
    }

    insert_nonfull(_root, tree_data_type_const(tmp.first, std::move(tmp.second)));
    ++_size;
    auto it = find(tmp.first);
    return std::make_pair(it, true);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::debug_print_tree() const
{
    if (_root == nullptr) {
        std::cerr << "TREE: empty\n";
        return;
    }
    std::cerr << "TREE: root_keys=[";
    for (auto &k : _root->_keys) std::cerr << k.first << ",";
    std::cerr << "]\n";
    
    // Print children recursively
    std::function<void(btree_node*, size_t)> print_node = [&](btree_node* node, size_t level) {
        if (node == nullptr || node->_pointers.empty()) return;
        for (size_t i = 0; i < node->_pointers.size(); ++i) {
            btree_node* child = node->_pointers[i];
            std::cerr << "  level=" << level << " child" << i << ": keys=[";
            for (auto &k : child->_keys) std::cerr << k.first << ",";
            std::cerr << "]\n";
            print_node(child, level + 1);
        }
    };
    print_node(_root, 1);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_iterator\n"
                          "B_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_iterator\n"
                          "B_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "template<typename... Args>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_iterator\n"
                          "B_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator pos)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_iterator\n"
                          "B_tree<tkey, tvalue, compare, t>::erase(btree_iterator pos)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator pos)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_iterator\n"
                          "B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator pos)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator beg, btree_iterator en)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_iterator\n"
                          "B_tree<tkey, tvalue, compare, t>::erase(btree_iterator beg, btree_iterator en)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator beg, btree_const_iterator en)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>\n"
                          "typename B_tree<tkey, tvalue, compare, t>::btree_iterator\n"
                          "B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator beg, btree_const_iterator en)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    // collect all elements in order
    std::vector<tree_data_type> elems;
    for (auto it = cbegin(); it != cend(); ++it)
        elems.emplace_back(*it);

    // find the key and determine successor key (if any)
    std::optional<tkey> succ_key;
    bool found = false;
    for (size_t i = 0; i < elems.size(); ++i)
    {
        if (!found && !compare_keys(elems[i].first, key) && !compare_keys(key, elems[i].first))
        {
            found = true;
            if (i + 1 < elems.size()) succ_key = elems[i+1].first;
            break;
        }
    }

    if (!found) return end();

    // rebuild tree without the key
    clear();
    for (auto &p : elems)
    {
        if (compare_keys(p.first, key) || compare_keys(key, p.first))
            emplace(p.first, p.second);
    }

    if (!succ_key.has_value()) return end();
    return find(*succ_key);
}

// endregion modifiers implementation


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return static_cast<const compare&>(*this)(lhs, rhs);
}


#endif