/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 23.01.2025
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

/**
 * @brief An enumeration of possible errors that can occur when adding or removing observers from an observable object.
 *
 */
enum class observer_errors {
    /**
     * @brief No error occurred.
     */
    ok = 0,

    /**
     * @brief The list of observers is full and cannot accept any more observers.
     */
    list_is_full,

    /**
     * @brief The observer is already registered with the observable object.
     */
    already_contains,

    /**
     * @brief The observer is not registered with the observable object.
     */
    not_found,

    /**
     * @brief The internal data structure of the observable object is broken.
     */
    internal_data_broken,

    /**
     * @brief A recursive notification call was detected.
     */
    notify_recursion_detected
};

}    // namespace xitren::comm
