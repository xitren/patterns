/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 03.03.2024
*/
#pragma once

#include <xitren/allocators/list_manager.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>

namespace xitren::allocators {

template <typename Type, size_t PoolSize>
class list_allocator {
    list_manager<PoolSize>& manager_;

    template <class U, size_t FriendPoolSize>
    friend class list_allocator;

public:
    using value_type      = Type;               // NOLINT
    using is_always_equal = std::false_type;    // NOLINT

    template <typename U>
    struct rebind    // NOLINT
    {
        using other = list_allocator<U, PoolSize>;    // NOLINT
    };

    constexpr explicit list_allocator(list_manager<PoolSize>& manager) : manager_{manager} {}

    template <class U>
    explicit list_allocator(list_allocator<U, PoolSize> const& other) noexcept : manager_{other.manager_}
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
        manager_.deallocate(ptr, size);
    }

    template <typename U>
    constexpr bool
    operator==(list_allocator<U, PoolSize> const& other) noexcept
    {
        return this == &other;
    }

    template <typename U>
    constexpr bool
    operator!=(list_allocator<U, PoolSize> const& other) noexcept
    {
        return !this->operator==(other);
    }
};

}    // namespace xitren::allocators
