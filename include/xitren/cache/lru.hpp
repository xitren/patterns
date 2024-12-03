/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 24.11.2024
*/
#pragma once
#include <xitren/cache/exceptions.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace xitren::cache {

template <class Key, class Value, std::size_t Size, bool Exception = true>
class lru {
    using timestamp          = std::chrono::time_point<std::chrono::system_clock>;
    using data_item          = std::tuple<Key, Value, timestamp>;
    using double_linked_list = std::list<data_item>;
    using hash_map           = std::unordered_map<Key, data_item>;
    using return_type        = std::optional<data_item>;

public:
    explicit lru(timestamp expired) noexcept : expired_after_{expired} {}

    void
    put(Key key, Value value)
    {
#ifdef PTHREAD_MUTEX_DEFAULT
        std::unique_lock<std::mutex> lock(access_);
#endif
        if (map_.contains(key)) {
            auto is_key = [key](data_item item) { return item.template get<0>() == key; };
            auto it     = std::find_if(list_.begin(), list_.end(), is_key);
            if (it != list_.end()) {
                auto obj = *it;
                list_.erase(it);
                list_.push_front(obj);
                map_[key] = obj;
            } else {
                if constexpr (Exception) {
                    throw cache_missed();
                }
            }
        } else {
            if (list_.size() >= Size) {
                list_.pop_back();
            }
            auto obj = data_item{key, value, get_time()};
            list_.push_front(obj);
            map_[key] = obj;
        }
    }

    return_type
    get(Key key)
    {
#ifdef PTHREAD_MUTEX_DEFAULT
        std::unique_lock<std::mutex> lock(access_);
#endif
        if (map_.contains(key)) {
            auto [value, time] = map_[key];
            if ((get_time() - time).count() >= expired_after_) {
                if constexpr (Exception) {
                    throw cache_timeout();
                }
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }
    }

    auto
    expired_after() const
    {
        return expired_after_;
    }

private:
    const timestamp    expired_after_;
    double_linked_list list_{};
    hash_map           map_{};
#ifdef PTHREAD_MUTEX_DEFAULT
    std::mutex access_{};
#endif

    inline timestamp
    get_time()
    {
        return std::chrono::system_clock::now();
    }
};

}    // namespace xitren::cache