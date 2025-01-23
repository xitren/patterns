/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 22.01.2025
*/
#pragma once

#include <xitren/comm/observer.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <mutex>
#include <ranges>
#include <variant>
#include <vector>

namespace xitren::comm::values {

template <class T>
class observable : public std::atomic<T>, public observable_dynamic<T> {

    T
    operator++(int) volatile noexcept
    {
        auto const ret = std::atomic<T>::fetch_add(1);
        notify_observers(ret);
        return ret;
    }

    T
    operator++(int) noexcept
    {
        auto const ret = std::atomic<T>::fetch_add(1);
        notify_observers(ret);
        return ret;
    }

    T
    operator--(int) volatile noexcept
    {
        auto const ret = std::atomic<T>::fetch_sub(1);
        notify_observers(ret);
        return ret;
    }

    T
    operator--(int) noexcept
    {
        auto const ret = std::atomic<T>::fetch_sub(1);
        notify_observers(ret);
        return ret;
    }

    T
    operator++() volatile noexcept
    {
        auto const ret = std::atomic<T>::fetch_add(1) + 1;
        notify_observers(ret);
        return ret;
    }

    T
    operator++() noexcept
    {
        auto const ret = std::atomic<T>::fetch_add(1) + 1;
        notify_observers(ret);
        return ret;
    }

    T
    operator--() volatile noexcept
    {
        auto const ret = std::atomic<T>::fetch_sub(1) + 1;
        notify_observers(ret);
        return ret;
    }

    T
    operator--() noexcept
    {
        auto const ret = std::atomic<T>::fetch_sub(1) + 1;
        notify_observers(ret);
        return ret;
    }

    T
    operator+=(int op) volatile noexcept
    {
        auto const ret = std::atomic<T>::fetch_add(op) + op;
        notify_observers(ret);
        return ret;
    }

    T
    operator+=(int op) noexcept
    {
        auto const ret = std::atomic<T>::fetch_add(op) + op;
        notify_observers(ret);
        return ret;
    }

    T
    operator-=(int op) volatile noexcept
    {
        auto const ret = std::atomic<T>::fetch_sub(op) - op;
        notify_observers(ret);
        return ret;
    }

    T
    operator-=(int op) noexcept
    {
        auto const ret = std::atomic<T>::fetch_sub(op) - op;
        notify_observers(ret);
        return ret;
    }

    T
    operator=(T op) volatile noexcept
    {
        std::atomic<T>::store(op);
        notify_observers(op);
        return op;
    }

    T
    operator=(T op) noexcept
    {
        std::atomic<T>::store(op);
        notify_observers(op);
        return op;
    }

    observable&
    operator=(observable const&)
        = delete;
    observable&
    operator=(observable const&) volatile
        = delete;
};

}    // namespace xitren::comm::values
