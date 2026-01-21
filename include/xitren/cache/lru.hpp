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
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <list>

namespace xitren::cache {

template <class Key, class Value, std::size_t Size, bool Exception = true>
class lru {
    using clock_type   = std::chrono::steady_clock;
    using time_point   = std::chrono::time_point<clock_type>;
    using duration_type = typename clock_type::duration;

    struct entry {
        Key       key;
        Value     value;
        time_point ts;
    };

    using list_type    = std::list<entry>;
    using iterator     = typename list_type::iterator;
    using map_type     = std::unordered_map<Key, iterator>;

public:
    using return_type = std::optional<Value>;

public:
    explicit lru(duration_type expired_after) noexcept : expired_after_{expired_after} {}

    void
    put(Key key, Value value)
    {
#ifdef PTHREAD_MUTEX_DEFAULT
        std::unique_lock<std::mutex> lock(access_);
#endif
        auto now = get_time();
        auto it  = map_.find(key);
        if (it != map_.end()) {
            // update + move to front
            it->second->value = std::move(value);
            it->second->ts    = now;
            list_.splice(list_.begin(), list_, it->second);
            return;
        }

        if (list_.size() >= Size) {
            // evict least recently used
            auto const& back = list_.back();
            map_.erase(back.key);
            list_.pop_back();
        }

        list_.push_front(entry{std::move(key), std::move(value), now});
        map_.emplace(list_.front().key, list_.begin());
    }

    return_type
    get(Key key)
    {
#ifdef PTHREAD_MUTEX_DEFAULT
        std::unique_lock<std::mutex> lock(access_);
#endif
        auto it = map_.find(key);
        if (it == map_.end()) {
            if constexpr (Exception) {
                throw cache_missed();
            }
            return std::nullopt;
        }

        auto now = get_time();
        if (expired_after_.count() > 0 && (now - it->second->ts) >= expired_after_) {
            // expired: remove entry
            list_.erase(it->second);
            map_.erase(it);
            if constexpr (Exception) {
                throw cache_timeout();
            }
            return std::nullopt;
        }

        // promote to most recently used
        list_.splice(list_.begin(), list_, it->second);
        return list_.front().value;
    }

    auto
    expired_after() const
    {
        return expired_after_;
    }

private:
    const duration_type expired_after_;
    list_type           list_{};
    map_type            map_{};
#ifdef PTHREAD_MUTEX_DEFAULT
    std::mutex access_{};
#endif

    inline time_point
    get_time()
    {
        return clock_type::now();
    }
};

}    // namespace xitren::cache