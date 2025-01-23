/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 03.03.2023
*/
#pragma once

#include <xitren/comm/observer_errors.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <mutex>
#include <ranges>
#include <variant>
#include <vector>

namespace xitren::comm {

template <typename T, bool Static = true, std::size_t Max = 8>
class observable;

template <typename T>
class observer {
public:
    /**
     * @brief Destructor
     */
    virtual ~observer() = default;

    /**
     * @brief This function is called when the observable object notifies the observer
     *
     * @param src pointer to the observable object
     * @param t1 the data that was passed to the notify function
     */
    void
    notification(void const* src, T const& t1) noexcept
    {
        data(src, t1);
    }

    /**
     * @brief This function is called when the observer is disconnected from the observable object
     *
     * @param src pointer to the observable object
     */
    virtual void
    disconnect(void const* /*src*/)
    {}

protected:
    /**
     * @brief This function is called when the observable object notifies the observer
     *
     * @param src pointer to the observable object
     * @param nd the data that was passed to the notify function
     */
    virtual void
    data(void const* src, T const& nd)
        = 0;
};

template <typename T, std::size_t Max>
class observable<T, true, Max> {
public:
    static constexpr uint32_t max_observers = Max;
    using observable_type                   = T;
    using size_type                         = std::size_t;
    using observer_list                     = std::array<observer<T>*, max_observers>;

public:
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
    add_observer(observer<observable_type>& observer) noexcept
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
    remove_observer(observer<observable_type> const& observer) noexcept
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
        if (inside) [[unlikely]]
            return observer_errors::notify_recursion_detected;
        inside = true;
        for (auto& item : std::views::counted(observers_.begin(), count_)) {
            item->notification(static_cast<void*>(this), n);
        }
        inside = false;
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
     * @brief A flag indicating whether a recursive notification call is being made
     */
    volatile bool inside{};

    /**
     * @brief Checks if an observer is registered with the observable object
     *
     * @param observer the observer to check
     * @return true if the observer is registered with the observable object
     * @return false if the observer is not registered with the observable object
     */
    constexpr bool
    contains(observer<observable_type> const& observer) noexcept
    {
        for (auto& item : std::views::counted(observers_.begin(), count_)) {
            if (item == &observer) [[unlikely]]
                return true;
        }
        return false;
    }
};

template <typename T, std::size_t Max>
using observable_static = observable<T, true, Max>;

template <typename T, std::size_t Size>
class observable<T, false, Size> {
public:
    /**
     * @brief The observable type
     */
    using observable_type = T;

    /**
     * @brief The size type
     */
    using size_type = std::size_t;

    /**
     * @brief The observer list
     */
    using observer_list = std::vector<observer<T>*>;

    /**
     * @brief The exception type
     */
    struct exception : public std::exception {
        /**
         * @brief Construct a new exception object
         *
         * @param err the error code
         */
        explicit exception(observer_errors err) : error{err} {};

        /**
         * @brief The error code
         */
        observer_errors error{};
    };

public:
    /**
     * @brief Default constructor
     */
    observable() noexcept = default;

    /**
     * @brief Destructor
     */
    virtual ~observable() noexcept { clear_observers(); }

    /**
     * @brief Add an observer to the observable
     *
     * @param observer the observer to add
     * @return observer_errors
     * - observer_errors::list_is_full if the list of observers is full and cannot accept any more observers
     * - observer_errors::already_contains if the observer is already registered with the observable object
     */
    void
    add_observer(observer<observable_type>& observer)
    {
#ifdef PTHREAD_MUTEX_DEFAULT
        std::unique_lock<std::mutex> lock(access_);
#endif
        if (!contains(observer)) [[likely]] {
            observers_.push_back(&observer);
        } else [[unlikely]] {
            throw exception{observer_errors::already_contains};
        }
    }

    /**
     * @brief Remove an observer from the observable
     *
     * @param observer the observer to remove
     * @return observer_errors
     * - observer_errors::internal_data_broken if the internal data structure of the observable object is broken
     * - observer_errors::not_found if the observer is not registered with the observable object
     */
    void
    remove_observer(observer<observable_type> const& observer)
    {
#ifdef PTHREAD_MUTEX_DEFAULT
        std::unique_lock<std::mutex> lock(access_);
#endif
        if (!std::erase(observers_, &observer)) {
            throw exception{observer_errors::not_found};
        }
    }

    /**
     * @brief Clear all observers from the observable
     */
    inline void
    clear_observers()
    {
#ifdef PTHREAD_MUTEX_DEFAULT
        std::unique_lock<std::mutex> lock(access_);
#endif
        for (auto& item : observers_) {
            item->disconnect(static_cast<void*>(this));
        }
        observers_.clear();
    }

    /**
     * @brief Notify all observers registered with the observable
     *
     * @param n the data to pass to the observers
     * @return observer_errors
     * - observer_errors::internal_data_broken if the internal data structure of the observable object is broken
     * - observer_errors::notify_recursion_detected if a recursive notification call was detected
     */
    void
    notify_observers(observable_type const& n)
    {
#ifdef PTHREAD_MUTEX_DEFAULT
        std::unique_lock<std::mutex> lock(access_);
#endif
        for (auto& item : observers_) {
            item->notification(static_cast<void*>(this), n);
        }
    }

private:
    observer_list observers_{};
#ifdef PTHREAD_MUTEX_DEFAULT
    std::mutex access_{};
#endif

    /**
     * @brief Check if an observer is registered with the observable
     *
     * @param observer the observer to check
     * @return true if the observer is registered with the observable
     * @return false if the observer is not registered with the observable
     */
    constexpr bool
    contains(observer<observable_type> const& observer) noexcept
    {
        for (auto& item : observers_) {
            if (item == &observer) [[unlikely]]
                return true;
        }
        return false;
    }
};
template <typename T>
using observable_dynamic = observable<T, false, 0>;

}    // namespace xitren::comm
