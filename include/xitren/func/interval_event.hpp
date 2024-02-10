#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <utility>
using namespace std::literals;

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

    interval_event(std::function<void(void)> function, time_type repeat_every = 100ms,
                   time_type wait_between_checks = 20ms)
        : function_{std::move(function)},
          thread_{[&] {
              std::this_thread::sleep_for(10ms);
              auto last_time{std::chrono::system_clock::now()};
              while (keep_running_.test_and_set()) {
                  if ((std::chrono::system_clock::now() - last_time).count() >= period_.count()) {
                      last_time += period_;
                      function_();
                  }
                  std::this_thread::sleep_for(period_between_checks_);
              }
              keep_running_.notify_one();
          }},
          period_{repeat_every},
          period_between_checks_{wait_between_checks}
    {
        thread_.detach();
    }

    ~interval_event() { stop(); }

    void
    stop()
    {
        if (!stop_) {
            keep_running_.clear();
            keep_running_.wait(false);
            stop_ = true;
        }
    }

    auto&
    thread() const noexcept
    {
        return thread_;
    }
    auto&
    period() const noexcept
    {
        return period_;
    }
    auto&
    period_between_checks() const noexcept
    {
        return period_between_checks_;
    }

    void
    period(time_type const& val) noexcept
    {
        period_ = val;
    }
    void
    period_between_checks(time_type const& val) noexcept
    {
        period_between_checks_ = val;
    }

private:
    std::function<void(void)> function_;
    std::atomic_flag          keep_running_{true};
    std::thread               thread_;
    time_type                 period_{};
    time_type                 period_between_checks_{};
    bool                      stop_{false};
};

}    // namespace xitren::func
