#pragma once

#include "monocle_config.hpp"

#include <memory>
#include <array>

namespace mon {

// a simple ring buffer queue for basic POD types
template <typename T, size_t STATIC_BUF_SIZE>
class RingQueue {

    static_assert(STATIC_BUF_SIZE > 0);
    static_assert((STATIC_BUF_SIZE & (STATIC_BUF_SIZE - 1)) == 0, "STATIC_BUF_SIZE must be a power of 2");

    std::array<T, STATIC_BUF_SIZE> init_arr;
    std::unique_ptr<T[]> mem{init_arr.data()};
    size_t index_mask = STATIC_BUF_SIZE - 1; // mem_size - 1, 1 less than a power of 2
    size_t first = 0;
    size_t n_elems = 0;

    void grow()
    {
        MON_ASSERT(n_elems == index_mask + 1);
        size_t new_mem_size = n_elems * 2;
        auto new_mem = std::make_unique_for_overwrite<T[]>(new_mem_size);
        if (!empty()) {
            auto it = std::copy_n(&mem[first], n_elems - first, &new_mem[0]);
            std::copy_n(&mem[0], first, it);
        }
        if (mem.get() == init_arr.data())
            mem.release();
        mem = std::move(new_mem);
        index_mask = new_mem_size - 1;
        first = 0;
    }

public:
    using value_type = T;

    RingQueue() = default;

    ~RingQueue()
    {
        if (mem.get() == init_arr.data())
            mem.release();
    }

    size_t size() const
    {
        return n_elems;
    }

    bool empty() const
    {
        return n_elems == 0;
    }

    void clear()
    {
        n_elems = 0;
    }

    void push_back(T x)
    {
        if (n_elems > index_mask) [[unlikely]]
            grow();
        size_t back = (first + n_elems++) & index_mask;
        mem[back] = x;
    }

    T& front()
    {
        MON_ASSERT(size() > 0);
        return mem[first];
    }

    void pop_front()
    {
        MON_ASSERT(size() > 0);
        first = (first + 1) & index_mask;
        --n_elems;
    }

    T operator[](size_t i) const
    {
        MON_ASSERT(i < size());
        return mem[(first + i) & index_mask];
    }

    class Iterator {
        const RingQueue* cont;
        size_t i;

    public:
        Iterator(const RingQueue* cont, size_t i) : cont{cont}, i{i} {}

        T operator*() const
        {
            return (*cont)[i];
        }

        Iterator& operator++()
        {
            ++i;
            return *this;
        }

        bool operator!=(Iterator o) const
        {
            return cont != o.cont || i != o.i;
        }
    };

    Iterator begin() const
    {
        return {this, 0};
    }

    Iterator end() const
    {
        return {this, size()};
    }
};

} // namespace mon
