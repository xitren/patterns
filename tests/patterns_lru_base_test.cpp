#include <xitren/cache/lru.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>

TEST(lru_test, simple_check)
{
    using namespace std::chrono_literals;
    xitren::cache::lru<int, std::string, 2, false> cache{50ms};

    cache.put(1, "one");
    cache.put(2, "two");

    auto v1 = cache.get(1);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, "one");

    // LRU order: 1 most-recent, 2 least-recent. Insert 3 should evict 2.
    cache.put(3, "three");
    EXPECT_FALSE(cache.get(2).has_value());
    EXPECT_TRUE(cache.get(1).has_value());
    EXPECT_TRUE(cache.get(3).has_value());

    // Expiration should remove entries.
    std::this_thread::sleep_for(80ms);
    EXPECT_FALSE(cache.get(1).has_value());
}
