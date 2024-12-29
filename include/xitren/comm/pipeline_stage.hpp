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
#include <functional>
#include <mutex>
#include <optional>
#include <ranges>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

namespace xitren::comm {

enum class pipeline_stage_exception : int { no_error = 0x00 };

template <class Type, class NextType, std::size_t BufferSize, func::log_adapter_concept Log>
class pipeline_stage {
    using atomic_cnt_type    = std::atomic<std::size_t>;
    using atomic_closed_type = std::atomic<bool>;
    using ready_type         = std::optional<Type>;
    using queue_type         = struct type_tag {
        std::array<ready_type, BufferSize> array;
        atomic_cnt_type                    tail;
    };
    using function_type = std::function<const NextType(pipeline_stage_exception, const Type)>;

public:
    pipeline_stage(function_type func) : func_{func} {}

    ~pipeline_stage()
    {
        closed_ = true;
        worker_.join();
    }

    void
    push(Type const&& data)
    {
        auto const i                          = queue_.tail++;
        queue_.array[i % queue_.array.size()] = data;
#ifdef DEBUG
        Log::trace() << "Index: " << queue_.tail << "\n";
#endif
    }

private:
    atomic_cnt_type    tail_{0};
    atomic_closed_type closed_{false};
    queue_type         queue_{};
    function_type      func_{};

    std::thread worker_ = std::thread{[this]() {
        using namespace std::chrono_literals;
#ifdef DEBUG
        Log::debug() << "Started thread... \n";
#endif
        while (!closed_ || ((queue_.tail - tail_) > 0)) {
            while (tail_ < queue_.tail) {
                if (!(queue_.array[(tail_) % queue_.array.size()])) {
                    std::this_thread::sleep_for(0ms);
#ifdef DEBUG
                    Log::trace() << "Data are not ready!\n";
#endif
                    continue;
                }
                auto const i = tail_++;
#ifdef DEBUG
                Log::trace() << "Index to process: " << i << "\n";
#endif
                auto& data = queue_.array[i % queue_.array.size()];
                func_(pipeline_stage_exception::no_error, data.value());
            }
            std::this_thread::sleep_for(0ms);
        }
#ifdef DEBUG
        Log::debug() << "All lines parsed: " << tail_ << "\n";
        Log::debug() << "End thread... \n";
#endif
    }};
};

}    // namespace xitren::comm
