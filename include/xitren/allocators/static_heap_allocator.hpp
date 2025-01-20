/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 03.03.2024
*/
#pragma once

#include <xitren/allocators/static_heap.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>

namespace xitren::allocators {

template <typename Type, size_t PoolSize>
class static_heap_allocator {
    static_heap<PoolSize>& manager_;

    template <class U, size_t FriendPoolSize>
    friend class static_heap_allocator;

public:
    using value_type      = Type;               // NOLINT
    using is_always_equal = std::false_type;    // NOLINT

    template <typename U>
    struct rebind                                            // NOLINT
    {
        using other = static_heap_allocator<U, PoolSize>;    // NOLINT
    };

    constexpr explicit static_heap_allocator(static_heap<PoolSize>& manager) : manager_{manager} {}

    template <class U>
    explicit static_heap_allocator(static_heap_allocator<U, PoolSize> const& other) noexcept : manager_{other.manager_}
    {}

    Type*
    allocate(size_t size)    // NOLINT
    {
        auto ptr = static_cast<Type*>(manager_.allocate(size * sizeof(Type)));
        if (ptr) {
            return ptr;
        }
        throw std::bad_alloc();
    }

    constexpr void
    deallocate(void* ptr, [[maybe_unused]] std::size_t size = 0) noexcept    // NOLINT
    {
        manager_.deallocate(ptr);
    }

    template <typename U>
    constexpr bool
    operator==(static_heap_allocator<U, PoolSize> const& other) noexcept
    {
        return this == &other;
    }

    template <typename U>
    constexpr bool
    operator!=(static_heap_allocator<U, PoolSize> const& other) noexcept
    {
        return !this->operator==(other);
    }
};

}    // namespace xitren::allocators
