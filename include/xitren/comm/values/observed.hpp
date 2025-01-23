/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 22.01.2025
*/
#pragma once

#include <xitren/comm/values/observable.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <mutex>
#include <ranges>
#include <variant>
#include <vector>

namespace xitren::comm::values {

template <class T>
class observable : protected std::atomic<T>, public observer<T> {
    T
    value()
    {
        return std::atomic<T>::load();
    }

protected:
    void
    data(void const* src, T const& nd)
    {
        std::atomic<T>::store(nd);
    }
};

}    // namespace xitren::comm::values
