/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 26.12.2024
*/
#pragma once

#include <xitren/func/log_adapter.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <ranges>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

namespace xitren::comm {

enum class exception : std::uint8_t { no_error = 0x00 };

template <std::size_t BufferSize, class Type, std::is_invocable<exception, Type> Worker, class Log>
class pipeline_stage {
    using atomic_cnt_type    = std::atomic<std::size_t>;
    using atomic_closed_type = std::atomic<bool>;
    using queue_type         = std::pair<std::array<Type, BufferSize>, atomic_cnt_type>;

public:
    ~pipeline_stage()
    {
        closed_ = true;
        worker_.join();
    }

private:
    atomic_cnt_type    tail_{0};
    atomic_closed_type closed_{false};
    queue_type         queue_{};

    std::thread worker_ = std::thread{[this]() {
        using namespace std::chrono_literals;
        Log::debug() << "Started thread... \n";
        while (!closed_) {
            while (tail_ < queue_.second) {
                auto& data = queue_.first[(tail_++) % queue_.first.size()];
                Worker(exception::no_error, data);
            }
            std::this_thread::sleep_for(0ms);
        }
        Log::debug() << "All lines parsed: " << tail_;
        Log::debug() << "End thread... ";
    }};
};

}    // namespace xitren::comm
