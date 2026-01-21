/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 30.12.2024
*/
#pragma once

#include <xitren/comm/pipeline_stage.hpp>
#include <xitren/func/log_adapter.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <mutex>
#include <optional>
#include <condition_variable>
#include <thread>
#include <utility>
#include <vector>

namespace xitren::comm {

struct measure_data {
    int id;
    int time;
    int load;
};

template <class Type, class NextType, std::size_t BufferSize, std::size_t PoolSize, func::log_adapter_concept Log>
class pipeline_stage_pool {
    static int const measure_points = 10;

    using ready_type         = std::optional<Type>;
    using statistics_type    = std::deque<measure_data>;
    using queue_type         = struct type_tag {
        std::array<ready_type, BufferSize> array{};
        statistics_type                    stat{};
        std::size_t                        head{0};
        std::size_t                        tail{0};
        std::atomic<std::size_t>           size{0};
        std::mutex                         mtx{};
        std::condition_variable            cv{};
        std::condition_variable            cv_space{};
    };
    using pool_type     = std::array<queue_type, PoolSize>;
    using thread_type   = std::vector<std::thread>;
    using function_type = std::function<NextType(pipeline_stage_exception, Type const&, measure_data)>;

public:
    pipeline_stage_pool(function_type func) : func_{func}, pool_size_{PoolSize}
    {
        auto thread = [this](int const pool_thread_n) {
            using namespace std::chrono_literals;
#ifdef DEBUG
            Log::debug() << "Started thread " << pool_thread_n << "... \n";
#endif
            auto& q = pool_[pool_thread_n];
            for (;;) {
                ready_type item;
                int        pending{};
                {
                    std::unique_lock<std::mutex> lock(q.mtx);
                    q.cv.wait(lock, [&] { return closed_ || q.size.load(std::memory_order_relaxed) > 0; });
                    if (closed_ && q.size.load(std::memory_order_relaxed) == 0) {
                        break;
                    }
                    item = std::move(q.array[q.head]);
                    q.array[q.head].reset();
                    q.head = (q.head + 1) % BufferSize;
                    auto const new_size = q.size.fetch_sub(1, std::memory_order_relaxed) - 1;
                    pending = static_cast<int>(new_size);
                    q.cv_space.notify_one();
                }
                if (!item) {
                    continue;
                }
                auto last_time{std::chrono::steady_clock::now()};
                func_(pipeline_stage_exception::no_error, item.value(),
                      measure_data{pool_thread_n, time_for_unit(q.stat), buffer_utilization(q.stat)});
                auto elapsed = std::chrono::steady_clock::now() - last_time;
                q.stat.push_front(measure_data{
                    pool_thread_n,
                    static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()),
                    pending});
                if (q.stat.size() > measure_points) {
                    q.stat.pop_back();
                }
            }
#ifdef DEBUG
            Log::debug() << "End thread " << pool_thread_n << "... \n";
#endif
        };

        for (std::size_t i{}; i < pool_size_; i++) {
            pool_threads_.push_back(std::thread(thread, i));
        }
    }

    ~pipeline_stage_pool()
    {
        closed_ = true;
        for (auto& q : pool_) {
            q.cv.notify_all();
            q.cv_space.notify_all();
        }
        for (auto& worker : pool_threads_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void
    push(Type&& data)
    {
        auto const min_id{min_thread()};
        auto&      q = pool_[min_id];
        std::unique_lock<std::mutex> lock(q.mtx);
        q.cv_space.wait(lock, [&] { return closed_ || q.size.load(std::memory_order_relaxed) < BufferSize; });
        if (closed_) {
            return;
        }
        q.array[q.tail] = std::move(data);
        q.tail          = (q.tail + 1) % BufferSize;
        q.size.fetch_add(1, std::memory_order_relaxed);
        q.cv.notify_one();
#ifdef DEBUG
        Log::trace() << "Queued[" << min_id << "]: " << q.size.load(std::memory_order_relaxed) << "\n";
#endif
    }

    void
    push(Type const& data)
    {
        auto const min_id{min_thread()};
        auto&      q = pool_[min_id];
        std::unique_lock<std::mutex> lock(q.mtx);
        q.cv_space.wait(lock, [&] { return closed_ || q.size.load(std::memory_order_relaxed) < BufferSize; });
        if (closed_) {
            return;
        }
        q.array[q.tail] = data;
        q.tail          = (q.tail + 1) % BufferSize;
        q.size.fetch_add(1, std::memory_order_relaxed);
        q.cv.notify_one();
#ifdef DEBUG
        Log::trace() << "Queued[" << min_id << "]: " << q.size.load(std::memory_order_relaxed) << "\n";
#endif
    }

    static int
    time_for_unit(statistics_type& stat)
    {
        int calc{};
        for (auto& item : stat) {
            calc += item.time;
        }
        return calc / measure_points;
    }

    static int
    buffer_utilization(statistics_type& stat)
    {
        int calc{};
        for (auto& item : stat) {
            calc += item.load;
        }
        return calc / measure_points;
    }

private:
    std::atomic<bool>  closed_{false};
    function_type      func_{};
    statistics_type    stat_{};
    std::size_t        pool_size_{};
    pool_type          pool_{};
    thread_type        pool_threads_{};

    int
    min_thread()
    {
        std::size_t min{std::numeric_limits<int>::max()};
        int         min_i{0};
        int         i{};
        for (auto& item : pool_) {
            auto const size{item.size.load(std::memory_order_relaxed)};
            if (min > size) {
                min   = size;
                min_i = i;
            }
            i++;
        }
        return min_i;
    }
};

}    // namespace xitren::comm
