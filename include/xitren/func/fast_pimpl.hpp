#pragma once

#include <new>
#include <type_traits>
#include <utility>

namespace xitren::func {

template <typename T, std::size_t Size, std::size_t Alignment, bool Strict = false>
class fast_pimpl final {
public:
    using type            = T;
    using pointer         = T*;
    using const_pointer   = T const*;
    using reference       = T&;
    using const_reference = T const&;

public:
    template <typename... Args>
    explicit fast_pimpl(Args&&... args) noexcept(noexcept(type{std::declval<Args>()...}))
    {
        new (get()) type(std::forward<Args>(args)...);
    }

    fast_pimpl(fast_pimpl&& rhs) noexcept(noexcept(type{std::declval<type>()})) : fast_pimpl{std::move(*rhs)} {}

    fast_pimpl(fast_pimpl const& rhs) noexcept(noexcept(type{std::declval<const_reference>})) : fast_pimpl{*rhs} {}

    ~fast_pimpl() noexcept
    {
        validator<sizeof(T), alignof(T)>::validate();
        get()->~type();
    }

    fast_pimpl&
    operator=(fast_pimpl const& rhs) noexcept(noexcept(std::declval<reference>() = std::declval<const_reference>()))
    {
        *get() = *rhs;
        return *this;
    }

    fast_pimpl&
    operator=(fast_pimpl&& rhs) noexcept(noexcept(std::declval<reference>() = std::declval<type>()))
    {
        *get() = std::move(*rhs);
        return *this;
    }

    pointer
    operator->() noexcept
    {
        return get();
    }
    const_pointer
    operator->() const noexcept
    {
        return get();
    }

    reference
    operator*() noexcept
    {
        return *get();
    }
    const_reference
    operator*() const noexcept
    {
        return *get();
    }

private:
    template <std::size_t ActualSize, std::size_t ActualAlignment>
    struct validator {
        static void
        validate() noexcept
        {
            static_assert(Size >= ActualSize, "incorrect specialization of Size: Size is less than sizeof(T)");
            static_assert(Size == ActualSize || !Strict,
                          "incorrect specialization of Size: Size and sizeof(T) mismatch");
            static_assert(Alignment % ActualAlignment == 0,
                          "incorrect specialization of Alignment: Alignment and "
                          "alignment_of(T) mismatch");
        }
    };

    pointer
    get()
    {
        return reinterpret_cast<pointer>(&storage_);
    }
    const_pointer
    get() const
    {
        return reinterpret_cast<const_pointer>(&storage_);
    }
    std::aligned_storage_t<Size, Alignment> storage_;
};

}    // namespace xitren::func
