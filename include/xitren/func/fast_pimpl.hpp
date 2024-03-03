/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 03.03.2023
*/
#pragma once

#include <new>
#include <type_traits>
#include <utility>

namespace xitren::func {

template <typename T, std::size_t Size, std::size_t Alignment, bool Strict = false>
class fast_pimpl final {
public:
    /**
     * @brief The type of the object managed by the fast_pimpl instance.
     */
    using type = T;

    /**
     * @brief A pointer to the object managed by the fast_pimpl instance.
     */
    using pointer = T*;

    /**
     * @brief A const pointer to the object managed by the fast_pimpl instance.
     */
    using const_pointer = T const*;

    /**
     * @brief A reference to the object managed by the fast_pimpl instance.
     */
    using reference = T&;

    /**
     * @brief A const reference to the object managed by the fast_pimpl instance.
     */
    using const_reference = T const&;

public:
    /**
     * @brief Constructs a fast_pimpl instance that manages an object of type T.
     *
     * @param args The arguments to be passed to the object's constructor.
     *
     * @tparam Args The types of the arguments to be passed to the object's constructor.
     */
    template <typename... Args>
    explicit fast_pimpl(Args&&... args) noexcept(noexcept(type{std::declval<Args>()...}))
    {
        new (get()) type(std::forward<Args>(args)...);
    }

    /**
     * @brief Move-constructs a fast_pimpl instance that manages an object of type T.
     *
     * @param rhs The fast_pimpl instance to be moved.
     */
    fast_pimpl(fast_pimpl&& rhs) noexcept(noexcept(type{std::declval<type>()})) : fast_pimpl{std::move(*rhs)} {}

    /**
     * @brief Copy-constructs a fast_pimpl instance that manages an object of type T.
     *
     * @param rhs The fast_pimpl instance to be copied.
     */
    fast_pimpl(fast_pimpl const& rhs) noexcept(noexcept(type{std::declval<const_reference>()})) : fast_pimpl{*rhs} {}

    /**
     * @brief Destroys the fast_pimpl instance and the object it manages.
     */
    ~fast_pimpl() noexcept
    {
        validator<sizeof(T), alignof(T)>::validate();
        get()->~type();
    }

    /**
     * @brief Assigns the value of another fast_pimpl instance to this instance.
     *
     * @param rhs The fast_pimpl instance whose value is to be assigned to this instance.
     *
     * @return A reference to this instance.
     */
    fast_pimpl&
    operator=(fast_pimpl const& rhs) noexcept(noexcept(std::declval<reference>() = std::declval<const_reference>()))
    {
        *get() = *rhs;
        return *this;
    }

    /**
     * @brief Assigns the value of a moved fast_pimpl instance to this instance.
     *
     * @param rhs The fast_pimpl instance whose value is to be moved to this instance.
     *
     * @return A reference to this instance.
     */
    fast_pimpl&
    operator=(fast_pimpl&& rhs) noexcept(noexcept(std::declval<reference>() = std::declval<type>()))
    {
        *get() = std::move(*rhs);
        return *this;
    }

    /**
     * @brief Returns a pointer to the object managed by the fast_pimpl instance.
     *
     * @return A pointer to the object managed by the fast_pimpl instance.
     */
    pointer
    operator->() noexcept
    {
        return get();
    }

    /**
     * @brief Returns a const pointer to the object managed by the fast_pimpl instance.
     *
     * @return A const pointer to the object managed by the fast_pimpl instance.
     */
    const_pointer
    operator->() const noexcept
    {
        return get();
    }

    /**
     * @brief Returns a reference to the object managed by the fast_pimpl instance.
     *
     * @return A reference to the object managed by the fast_pimpl instance.
     */
    reference
    operator*() noexcept
    {
        return *get();
    }

    /**
     * @brief Returns a const reference to the object managed by the fast_pimpl instance.
     *
     * @return A const reference to the object managed by the fast_pimpl instance.
     */
    const_reference
    operator*() const noexcept
    {
        return *get();
    }

private:
    /**
     * @brief A helper struct that validates the specializations of Size and Alignment.
     */
    template <std::size_t ActualSize, std::size_t ActualAlignment>
    struct validator {
        /**
         * @brief A static function that validates the specializations of Size and Alignment.
         */
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

    /**
     * @brief Returns a pointer to the object managed by the fast_pimpl instance.
     *
     * @return A pointer to the object managed by the fast_pimpl instance.
     */
    pointer
    get()
    {
        return reinterpret_cast<pointer>(&storage_);
    }

    /**
     * @brief Returns a const pointer to the object managed by the fast_pimpl instance.
     *
     * @return A const pointer to the object managed by the fast_pimpl instance.
     */
    const_pointer
    get() const
    {
        return reinterpret_cast<const_pointer>(&storage_);
    }

    /**
     * @brief The storage for the object managed by the fast_pimpl instance.
     */
    std::aligned_storage_t<Size, Alignment> storage_;
};

}    // namespace xitren::func
