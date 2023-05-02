#include <iterator>
#include <memory>
#include <type_traits>

template <size_t N>
class StackStorage {
  private:
    uint8_t arr[N];
    size_t pos = 0;

  public:
    StackStorage() {}
    template <typename T>
    T* allocate(size_t count) {
        void* ans = arr + pos;
        size_t space = N - pos;
        if (std::align(alignof(T), count * sizeof(T), ans, space) == nullptr) {
            throw std::bad_alloc();
        }
        pos += count * sizeof(T);
        return reinterpret_cast<T*>(ans);
    }
};

template <typename T, size_t N>
class StackAllocator {
  public:
    StackStorage<N>* storage;

    using value_type = T;
    StackAllocator() {}
    StackAllocator(StackStorage<N>& storage)
        : storage(&storage) {}

    template <typename U>
    StackAllocator(const StackAllocator<U, N>& other)
        : storage(other.storage) {}

    template <typename U>
    StackAllocator& operator=(const StackAllocator<U, N>& other) {
        storage = other.storage;
        return *this;
    }

    template <typename U>
    bool operator==(const StackAllocator<U, N>& other) const {
        return storage == other.storage;
    }

    template <typename U>
    bool operator!=(const StackAllocator<U, N>& other) const {
        return storage != other.storage;
    }

    ~StackAllocator() {}

    template <typename U>
    struct rebind {
        using other = StackAllocator<U, N>;
    };

    T* allocate(size_t count) {
        return storage->template allocate<T>(count);
    }
    void deallocate(T* /*unused*/, size_t /*unused*/) {}
};
