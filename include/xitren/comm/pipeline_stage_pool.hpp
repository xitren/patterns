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
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <ranges>
#include <thread>
#include <utility>
#include <variant>
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

    using atomic_cnt_type    = std::atomic<std::size_t>;
    using atomic_closed_type = std::atomic<bool>;
    using ready_type         = std::optional<Type>;
    using statistics_type    = std::deque<measure_data>;
    using queue_type         = struct type_tag {
        std::array<ready_type, BufferSize> array;
        statistics_type                    stat;
        atomic_cnt_type                    tail;
        atomic_cnt_type                    head;
    };
    using pool_type     = std::array<queue_type, 8>;
    using thread_type   = std::vector<std::thread>;
    using function_type = std::function<const NextType(pipeline_stage_exception, const Type, const measure_data)>;

public:
    pipeline_stage_pool(function_type func) : func_{func}, pool_size_{PoolSize}
    {
        auto thread = [this](int const pool_thread_n) {
            using namespace std::chrono_literals;
#ifdef DEBUG
            Log::debug() << "Started thread " << pool_thread_n << "... \n";
#endif
            auto& [array_l, stat_l, tail_l, head_l] = pool_[pool_thread_n];

            while (!closed_ || ((tail_l - head_l) > 0)) {
                while (head_l < tail_l) {
                    if (!(array_l[(head_l) % array_l.size()])) {
                        std::this_thread::sleep_for(0ms);
#ifdef DEBUG
                        Log::trace() << "[" << pool_thread_n << "] Data are not ready!\n";
#endif
                        continue;
                    }
                    auto const i = head_l++;
#ifdef DEBUG
                    Log::trace() << "[" << pool_thread_n << "] Index to process: " << i << "\n";
#endif
                    auto& data = array_l[i % array_l.size()];
                    auto  last_time{std::chrono::system_clock::now()};
                    func_(pipeline_stage_exception::no_error, data.value(),
                          measure_data{pool_thread_n, time_for_unit(stat_l), buffer_utilization(stat_l)});
                    auto elapsed = std::chrono::system_clock::now() - last_time;
                    stat_l.push_front(measure_data{
                        pool_thread_n,
                        static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()),
                        static_cast<int>(tail_l - head_l)});
                    if (stat_l.size() > measure_points) {
                        stat_l.pop_back();
                    }
                }
                std::this_thread::sleep_for(0ms);
            }
#ifdef DEBUG
            Log::debug() << "End thread " << pool_thread_n << "... \n";
#endif
        };

        for (int i{}; i < pool_size_; i++) {
            pool_threads_.push_back(std::thread(thread, i));
        }
    }

    ~pipeline_stage_pool()
    {
        closed_ = true;
        for (auto& worker : pool_threads_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void
    push(Type const&& data)
    {
        auto const min_id{min_thread()};
        auto const i                                        = pool_[min_id].tail++;
        pool_[min_id].array[i % pool_[min_id].array.size()] = data;
#ifdef DEBUG
        Log::trace() << "Index[" << min_id << "]: " << pool_[min_id].tail << "\n";
#endif
    }

    void
    push(Type const& data)
    {
        auto const min_id{min_thread()};
        auto const i                                        = pool_[min_id].tail++;
        pool_[min_id].array[i % pool_[min_id].array.size()] = data;
#ifdef DEBUG
        Log::trace() << "Index[" << min_id << "]: " << pool_[min_id].tail << "\n";
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
    atomic_closed_type closed_{false};
    function_type      func_{};
    statistics_type    stat_{};
    std::size_t        pool_size_{};
    pool_type          pool_{};
    thread_type        pool_threads_{};

    int
    min_thread()
    {
        int min{std::numeric_limits<int>::max()};
        int min_i{0};
        int i{};
        for (auto& item : pool_) {
            auto const size{item.tail - item.head};
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
