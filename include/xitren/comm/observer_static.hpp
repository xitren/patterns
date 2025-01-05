/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 05.01.2024
*/
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <mutex>
#include <ranges>
#include <variant>
#include <vector>

namespace xitren::comm {

template <typename D>
class observable_impl {};

template <typename D, typename T>
class observer {
public:
    inline void
    notification(observable_impl<D> const& src, D const& data) noexcept
    {
        static_cast<T*>(this)->notification_impl(src, data);
    }
};

template <typename D, typename T>
concept observer_concept = requires { std::is_base_of<observer<D, T>, T>(); };

template <typename D, observer_concept<D> Observer, observer_concept<D>... Observers>
class observable {
    using next_type = observable<D, Observers...>;

    next_type next_;
    Observer& obs_;

public:
    observable_impl<D>& root;
    constexpr explicit observable(Observer& rel, Observers&... obs)
        : next_(std::forward<Observers&>(obs)...), obs_{rel}, root{next_.root} {};
    observable(observable const&) = delete;
    observable(observable&&)      = delete;
    observable const&
    operator=(observable const&)
        = delete;
    ~observable() = default;

    inline void
    notify(D const& data) noexcept
    {
        obs_.notification(root, data);
        return next_.notify(data);
    }
};

template <typename D, observer_concept<D> Observer>
class observable<D, Observer> {
protected:
    Observer& obs_;

public:
    observable_impl<D> root{};
    constexpr explicit observable(Observer& rel) : obs_{rel} {};
    observable(observable const&) = delete;
    observable(observable&&)      = delete;
    observable const&
    operator=(observable const&)
        = delete;
    ~observable() = default;

    inline void
    notify(D const& data) noexcept
    {
        obs_.notification(root, data);
    }
};

}    // namespace xitren::comm
