/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 29.12.2024
*/
#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <utility>

namespace xitren::func {

template <typename T>
concept log_adapter_concept = requires {
    T::trace();
    T::debug();
    T::warning();
    T::error();
    T::trace();
};

}    // namespace xitren::func