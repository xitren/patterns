/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 03.03.2023
*/
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <thread>
#include <utility>

namespace xitren::func {

class interval_event {
    using time_type = std::chrono::milliseconds;

public:
    interval_event() = delete;
    interval_event&
    operator=(interval_event const& other)
        = delete;
    interval_event&
    operator=(interval_event const&& other)
        = delete;
    interval_event(interval_event const& val) = delete;
    interval_event(interval_event&& val)      = delete;

    /**
     * Constructs a new interval_event object.
     *
     * @param function The function to be called periodically.
     * @param repeat_every The interval at which the function should be called, in milliseconds.
     * @param wait_between_checks The interval between checks for whether the function should be called, in
     * milliseconds.
     */
    interval_event(std::function<void(void)> function, time_type repeat_every = time_type{100},
                   time_type wait_between_checks = time_type{20})
        : function_{std::move(function)}
    {
        period_ms_.store(repeat_every.count(), std::memory_order_relaxed);
        check_ms_.store(wait_between_checks.count(), std::memory_order_relaxed);
        thread_ = std::thread([this]() {
            using clock = std::chrono::steady_clock;
            auto last_time = clock::now();
            while (running_.load(std::memory_order_acquire)) {
                auto period = time_type{period_ms_.load(std::memory_order_relaxed)};
                if (period.count() <= 0) {
                    period = time_type{1};
                }
                auto const next_time = last_time + period;
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    cv_.wait_until(lock, next_time, [&] { return !running_.load(std::memory_order_acquire); });
                }
                if (!running_.load(std::memory_order_acquire)) {
                    break;
                }

                // Catch up if we were delayed (or if period was changed inside callback).
                int guard = 0;
                while (running_.load(std::memory_order_acquire) && guard++ < 100) {
                    period = time_type{period_ms_.load(std::memory_order_relaxed)};
                    if (period.count() <= 0) {
                        period = time_type{1};
                    }
                    auto const target = last_time + period;
                    if (clock::now() < target) {
                        break;
                    }
                    last_time = target;
                    function_();
                }
            }
        });
    }

    /**
     * Destroys the interval_event object.
     */
    ~interval_event() { stop(); }

    /**
     * Stops the interval_event object.
     */
    void
    stop()
    {
        bool expected = true;
        if (running_.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) {
            cv_.notify_all();
            if (thread_.joinable()) {
                thread_.join();
            }
        }
    }

    /**
     * Returns a reference to the thread object that is used to run the interval_event.
     */
    auto&
    thread() const noexcept
    {
        return thread_;
    }

    /**
     * Returns the interval at which the function is called, in milliseconds.
     */
    auto&
    period() const noexcept
    {
        period_cache_ = time_type{period_ms_.load(std::memory_order_relaxed)};
        return period_cache_;
    }

    /**
     * Returns the interval between checks for whether the function should be called, in milliseconds.
     */
    auto&
    period_between_checks() const noexcept
    {
        check_cache_ = time_type{check_ms_.load(std::memory_order_relaxed)};
        return check_cache_;
    }

    /**
     * Sets the interval at which the function is called.
     *
     * @param val The new interval, in milliseconds.
     */
    void
    period(time_type const& val) noexcept
    {
        period_ms_.store(val.count(), std::memory_order_relaxed);
    }

    /**
     * Sets the interval between checks for whether the function should be called.
     *
     * @param val The new interval, in milliseconds.
     */
    void
    period_between_checks(time_type const& val) noexcept
    {
        check_ms_.store(val.count(), std::memory_order_relaxed);
    }

private:
    std::function<void(void)> function_;
    std::atomic<bool>         running_{true};
    std::thread               thread_{};
    std::mutex                mtx_{};
    std::condition_variable   cv_{};
    std::atomic<time_type::rep> period_ms_{0};
    std::atomic<time_type::rep> check_ms_{0};
    mutable time_type         period_cache_{};
    mutable time_type         check_cache_{};
};

}    // namespace xitren::func
