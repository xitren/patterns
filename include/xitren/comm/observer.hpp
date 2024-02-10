#pragma once

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
    virtual ~observer() = default;

    void
    notification(void const* src, T const& t1) noexcept
    {
        data(src, t1);
    }

    virtual void
    disconnect(void const* /*src*/)
    {}

protected:
    virtual void
    data(void const* src, T const& nd)
        = 0;
};

enum class observer_errors {
    ok = 0,
    list_is_full,
    already_contains,
    not_found,
    internal_data_broken,
    notify_recursion_detected
};

template <typename T, std::size_t Max>
class observable<T, true, Max> {
public:
    static constexpr uint32_t max_observers = Max;
    using observable_type                   = T;
    using size_type                         = std::size_t;
    using observer_list                     = std::array<observer<T>*, max_observers>;

public:
    observable() noexcept = default;
    virtual ~observable() noexcept { clear_observers(); }

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

    inline void
    clear_observers() noexcept
    {
        for (auto& item : std::views::counted(observers_.begin(), count_)) {
            item->disconnect(static_cast<void*>(this));
        }
        count_ = 0;
    }

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
    observer_list observers_{};
    size_type     count_{0};
    volatile bool inside{};

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
    using observable_type = T;
    using size_type       = std::size_t;
    using observer_list   = std::vector<observer<T>*>;
    struct exception : public std::exception {
        explicit exception(observer_errors err) : error{err} {};
        observer_errors error{};
    };

public:
    observable() noexcept = default;
    virtual ~observable() noexcept { clear_observers(); }

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
