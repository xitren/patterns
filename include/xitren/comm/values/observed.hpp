/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 22.01.2025
*/
#pragma once

#include <xitren/comm/values/pre_def.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <ranges>
#include <variant>

namespace xitren::comm::values {

template <class T>
class observed : protected std::atomic<T> {
    using function_type = std::function<void(T const)>;

public:
    observed() = default;
    explicit observed(function_type function) : function_{std::move(function)} {};
    observed(observed const&) = delete;
    observed(observed&&)      = delete;
    observed const&
    operator=(observed const&)
        = delete;

    void
    on_update(function_type function)
    {
        function_ = function;
    }

    T
    value()
    {
        return std::atomic<T>::load();
    }

    /**
     * @brief Destructor
     */
    virtual ~observed() = default;

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
    void
    data(void const* src, T const& nd)
    {
        std::atomic<T>::store(nd);
        if (function_) {
            function_(nd);
        }
    }

private:
    function_type function_{nullptr};
};

}    // namespace xitren::comm::values
