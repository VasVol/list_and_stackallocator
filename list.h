#include <iterator>
#include <memory>
#include <type_traits>

template <typename T, typename Alloc = std::allocator<T>>
class List {
  private:
    struct BaseNode {
        BaseNode* prev;
        BaseNode* next;
        BaseNode()
            : prev(this), next(this) {}
        BaseNode(BaseNode* prev, BaseNode* next)
            : prev(prev), next(next) {}
    };
    struct Node : BaseNode {
        T value;
        Node(BaseNode* prev, BaseNode* next, const T& value)
            : BaseNode(prev, next), value(value) {}
        Node(BaseNode* prev, BaseNode* next)
            : BaseNode(prev, next), value() {}
    };

    using AllocTraits = std::allocator_traits<Alloc>;
    using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
    using NodeAllocTraits = std::allocator_traits<NodeAlloc>;
    Alloc alloc = Alloc();

    BaseNode fakeNode = BaseNode();
    size_t sz = 0;

  public:
    template <bool IsConst>
    class common_iterator {
      public:
        BaseNode* node_ptr;

        using difference_type = ptrdiff_t;
        using value_type = std::conditional_t<IsConst, const T, T>;
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using reference = std::conditional_t<IsConst, const T&, T&>;
        using iterator_category = std::bidirectional_iterator_tag;

        common_iterator()
            : node_ptr() {}
        explicit common_iterator(BaseNode* node_ptr)
            : node_ptr(node_ptr) {}

        operator common_iterator<true>() const {
            common_iterator<true> iter;
            iter.node_ptr = this->node_ptr;
            return iter;
        }
        operator common_iterator<false>() const = delete;

        template <bool IsConst2>
        bool operator==(common_iterator<IsConst2> other) const {
            return node_ptr == other.node_ptr;
        }
        template <bool IsConst2>
        bool operator!=(common_iterator<IsConst2> other) const {
            return node_ptr != other.node_ptr;
        }
        reference operator*() {
            return static_cast<Node*>(node_ptr)->value;
        }
        common_iterator& operator++() {
            node_ptr = node_ptr->next;
            return *this;
        }
        common_iterator operator++(int) {
            auto tmp_iter = *this;
            ++*this;
            return tmp_iter;
        }
        common_iterator& operator--() {
            node_ptr = node_ptr->prev;
            return *this;
        }
        common_iterator operator--(int) {
            auto tmp_iter = *this;
            --*this;
            return tmp_iter;
        }
    };
    using iterator = common_iterator<false>;
    using const_iterator = common_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() {
        return iterator(fakeNode.next);
    }
    iterator end() {
        return iterator(fakeNode.next->prev);
    }
    const_iterator cbegin() const {
        return iterator(fakeNode.next);
    }
    const_iterator cend() const {
        return iterator(fakeNode.next->prev);
    }
    reverse_iterator rbegin() {
        return std::make_reverse_iterator(end());
    }
    reverse_iterator rend() {
        return std::make_reverse_iterator(begin());
    }
    const_reverse_iterator crbegin() const {
        return std::make_reverse_iterator(cend());
    }
    const_reverse_iterator crend() const {
        return std::make_reverse_iterator(cbegin());
    }

    const_iterator begin() const {
        return cbegin();
    }
    const_iterator end() const {
        return cend();
    }
    const_reverse_iterator rbegin() const {
        return crbegin();
    }
    const_reverse_iterator rend() const {
        return crend();
    }

    List() {}
    List(size_t count, const Alloc& alloc = Alloc())
        : alloc(alloc) {
        size_t i = 0;
        try {
            for (; i < count; ++i) {
                insert_helper(end());
            }
        } catch (...) {
            for (size_t j = 0; j < i; ++j) {
                pop_back();
            }
            throw;
        }
    }
    List(size_t count, const T& value, const Alloc& alloc = Alloc())
        : alloc(alloc) {
        size_t i = 0;
        try {
            for (; i < count; ++i) {
                push_back(value);
            }
        } catch (...) {
            for (size_t j = 0; j < i; ++j) {
                pop_back();
            }
            throw;
        }
    }
    List(const List& other)
        : alloc(AllocTraits::select_on_container_copy_construction(other.alloc)) {
        try {
            for (const auto& elem : other) {
                push_back(elem);
            }
        } catch (...) {
            while (sz > 0) {
                pop_back();
            }
            throw;
        }
    }
    List(const Alloc& alloc)
        : alloc(alloc) {}
    List& operator=(const List& other) {
        size_t prev_sz = sz;
        Alloc prev_alloc = alloc;
        if (AllocTraits::propagate_on_container_copy_assignment::value && (alloc != other.alloc)) {
            alloc = other.alloc;
        }
        Alloc new_alloc = alloc;
        for (const auto& elem : other) {
            try {
                push_back(elem);
            } catch (...) {
                while (sz > prev_sz) {
                    pop_back();
                }
                alloc = prev_alloc;
                throw;
            }
        }
        alloc = prev_alloc;
        while (sz > other.sz) {
            pop_front();
        }
        alloc = new_alloc;
        return *this;
    }
    ~List() {
        while (sz > 0) {
            pop_back();
        }
    }

    size_t size() const {
        return sz;
    }
    void push_back(const T& value) {
        insert(end(), value);
    }
    void pop_back() {
        erase(--end());
    }
    void push_front(const T& value) {
        insert(begin(), value);
    }
    void pop_front() {
        erase(begin());
    }

    iterator insert(const_iterator it, const T& value) {
        return insert_helper(it, value);
    }

    iterator erase(const_iterator iter) {
        NodeAlloc node_alloc = NodeAlloc(alloc);
        Node* ptr = static_cast<Node*>(iter.node_ptr);
        ptr->next->prev = ptr->prev;
        ptr->prev->next = ptr->next;
        auto ans_iter = iterator(ptr->next);
        NodeAllocTraits::destroy(node_alloc, ptr);
        NodeAllocTraits::deallocate(node_alloc, ptr, 1);
        --sz;
        return ans_iter;
    }

    Alloc get_allocator() const {
        return alloc;
    }

  private:
    template <typename... Args>
    iterator insert_helper(const_iterator it, const Args&... args) {
        NodeAlloc node_alloc = NodeAlloc(alloc);
        Node* ptr = NodeAllocTraits::allocate(node_alloc, 1);
        try {
            NodeAllocTraits::construct(node_alloc, ptr, it.node_ptr->prev, it.node_ptr, args...);
        } catch (...) {
            NodeAllocTraits::deallocate(node_alloc, ptr, 1);
            throw;
        }
        it.node_ptr->prev->next = ptr;
        it.node_ptr->prev = ptr;
        ++sz;
        return iterator(ptr);
    }
};
