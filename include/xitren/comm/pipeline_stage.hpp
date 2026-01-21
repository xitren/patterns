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
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <condition_variable>
#include <deque>
#include <thread>
#include <utility>

namespace xitren::comm {

enum class pipeline_stage_exception : int { no_error = 0x00 };

template <class Type, class NextType, std::size_t BufferSize, func::log_adapter_concept Log>
class pipeline_stage {
    static int const measure_points = 10;

    using ready_type         = std::optional<Type>;
    using measure_type       = std::pair<int, int>;
    using statistics_type    = std::deque<measure_type>;
    using queue_type         = std::array<ready_type, BufferSize>;
    using function_type      = std::function<NextType(pipeline_stage_exception, Type const&, measure_type)>;

public:
    pipeline_stage(function_type func) : func_{func} {}

    ~pipeline_stage()
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            closed_ = true;
        }
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    void
    push(Type&& data)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_space_.wait(lock, [&] { return closed_ || size_ < BufferSize; });
        if (closed_) {
            return;
        }
        queue_[tail_] = std::move(data);
        tail_         = (tail_ + 1) % BufferSize;
        ++size_;
        cv_.notify_one();
#ifdef DEBUG
        Log::trace() << "Queued size: " << size_ << "\n";
#endif
    }

    void
    push(Type const& data)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_space_.wait(lock, [&] { return closed_ || size_ < BufferSize; });
        if (closed_) {
            return;
        }
        queue_[tail_] = data;
        tail_         = (tail_ + 1) % BufferSize;
        ++size_;
        cv_.notify_one();
#ifdef DEBUG
        Log::trace() << "Queued size: " << size_ << "\n";
#endif
    }

    auto
    time_for_unit() const
    {
        int calc{};
        for (auto& item : stat_) {
            calc += item.first;
        }
        return calc / measure_points;
    }

    auto
    buffer_utilization() const
    {
        int calc{};
        for (auto& item : stat_) {
            calc += item.second;
        }
        return calc / measure_points;
    }

private:
    mutable std::mutex              mtx_{};
    std::condition_variable         cv_{};
    std::condition_variable         cv_space_{};
    bool                            closed_{false};
    queue_type                      queue_{};
    std::size_t                     head_{0};
    std::size_t                     tail_{0};
    std::size_t                     size_{0};
    function_type      func_{};
    statistics_type    stat_{};

    std::thread worker_ = std::thread{[this]() {
        using namespace std::chrono_literals;
#ifdef DEBUG
        Log::debug() << "Started thread... \n";
#endif
        for (;;) {
            ready_type item;
            int        pending{};
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [&] { return closed_ || size_ > 0; });
                if (closed_ && size_ == 0) {
                    break;
                }
                item = std::move(queue_[head_]);
                queue_[head_].reset();
                head_ = (head_ + 1) % BufferSize;
                --size_;
                pending = static_cast<int>(size_);
                cv_space_.notify_one();
            }

            if (!item) {
                continue;
            }

            auto last_time{std::chrono::steady_clock::now()};
            func_(pipeline_stage_exception::no_error, item.value(), measure_type{time_for_unit(), buffer_utilization()});
            auto elapsed = std::chrono::steady_clock::now() - last_time;

            stat_.push_front(
                measure_type{static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()),
                             pending});
            if (stat_.size() > measure_points) {
                stat_.pop_back();
            }
        }
#ifdef DEBUG
        Log::debug() << "End thread... \n";
        Log::debug() << "End thread... \n";
#endif
    }};
};

}    // namespace xitren::comm
