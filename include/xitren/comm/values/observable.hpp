/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 22.01.2025
*/
#pragma once

#include <xitren/comm/observer_errors.hpp>
#include <xitren/comm/values/observed.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <ranges>
#include <variant>

namespace xitren::comm::values {

template <class T, std::size_t Max>
class observable : protected std::atomic<T> {
public:
    static constexpr uint32_t max_observers = Max;
    using observable_type                   = T;
    using size_type                         = std::size_t;
    using observer_list                     = std::array<observed<observable_type>*, max_observers>;

    T
    value()
    {
        return std::atomic<T>::load();
    }

    observable_type
    operator++(int) volatile noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_add(1);
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator++(int) noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_add(1);
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator--(int) volatile noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_sub(1);
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator--(int) noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_sub(1);
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator++() volatile noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_add(1) + 1;
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator++() noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_add(1) + 1;
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator--() volatile noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_sub(1) + 1;
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator--() noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_sub(1) + 1;
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator+=(int op) volatile noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_add(op) + op;
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator+=(int op) noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_add(op) + op;
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator-=(int op) volatile noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_sub(op) - op;
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator-=(int op) noexcept
    {
        auto const ret = std::atomic<observable_type>::fetch_sub(op) - op;
        notify_observers(ret);
        return ret;
    }

    observable_type
    operator=(observable_type op) volatile noexcept
    {
        std::atomic<observable_type>::store(op);
        notify_observers(op);
        return op;
    }

    observable_type
    operator=(observable_type op) noexcept
    {
        std::atomic<observable_type>::store(op);
        notify_observers(op);
        return op;
    }

    observable&
    operator=(observable const&)
        = delete;
    observable&
    operator=(observable const&) volatile
        = delete;

    /**
     * @brief Constructs a new observable object
     */
    observable() noexcept = default;

    /**
     * @brief Destroys the observable object
     */
    virtual ~observable() noexcept { clear_observers(); }

    /**
     * @brief Adds an observer to the observable object
     *
     * @param observer the observer to add
     * @return observer_errors
     * - observer_errors::list_is_full if the list of observers is full and cannot accept any more observers
     * - observer_errors::already_contains if the observer is already registered with the observable object
     */
    observer_errors
    add_observer(observed<observable_type>& observer) noexcept
    {
        if (count_ >= max_observers) [[unlikely]]
            return observer_errors::list_is_full;
        if (!contains(observer)) [[likely]] {
            observers_[count_++] = &observer;
        } else [[unlikely]] {
            return observer_errors::already_contains;
        }
        return observer_errors::ok;
    }

    /**
     * @brief Removes an observer from the observable object
     *
     * @param observer the observer to remove
     * @return observer_errors
     * - observer_errors::internal_data_broken if the internal data structure of the observable object is broken
     * - observer_errors::not_found if the observer is not registered with the observable object
     */
    observer_errors
    remove_observer(observed<observable_type> const& observer) noexcept
    {
        if (count_ > max_observers) [[unlikely]]
            return observer_errors::internal_data_broken;
        if (count_ == 0) [[unlikely]]
            return observer_errors::not_found;
        auto it = std::remove(observers_.begin(), observers_.begin() + count_, &observer);
        if ((observers_.begin() + count_) - it) {
            count_ = it - observers_.begin();
            return observer_errors::ok;
        } else {
            return observer_errors::not_found;
        }
    }

    /**
     * @brief Clears all observers from the observable object
     */
    inline void
    clear_observers() noexcept
    {
        for (auto& item : std::views::counted(observers_.begin(), count_)) {
            item->disconnect(static_cast<void*>(this));
        }
        count_ = 0;
    }

    /**
     * @brief Notifies all observers registered with the observable object
     *
     * @param n the data to pass to the observers
     * @return observer_errors
     * - observer_errors::internal_data_broken if the internal data structure of the observable object is broken
     * - observer_errors::notify_recursion_detected if a recursive notification call was detected
     */
    observer_errors
    notify_observers(observable_type const& n) noexcept
    {
        if (count_ > max_observers) [[unlikely]]
            return observer_errors::internal_data_broken;
        for (auto& item : std::views::counted(observers_.begin(), count_)) {
            item->notification(static_cast<void*>(this), n);
        }
        return observer_errors::ok;
    }

private:
    /**
     * @brief The list of observers registered with the observable object
     */
    observer_list observers_{};
    /**
     * @brief The number of observers registered with the observable object
     */
    size_type count_{0};

    /**
     * @brief Checks if an observer is registered with the observable object
     *
     * @param observer the observer to check
     * @return true if the observer is registered with the observable object
     * @return false if the observer is not registered with the observable object
     */
    constexpr bool
    contains(observed<observable_type> const& observer) noexcept
    {
        for (auto& item : std::views::counted(observers_.begin(), count_)) {
            if (item == &observer) [[unlikely]]
                return true;
        }
        return false;
    }
};

}    // namespace xitren::comm::values
