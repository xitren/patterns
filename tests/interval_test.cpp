#include <xitren/func/interval_event.hpp>

#include <gtest/gtest.h>

#include <iostream>

using namespace xitren::func;
using namespace std::chrono;

TEST(interval_test, simple_check)
{
    auto                       start_time{system_clock::now()};
    std::atomic<std::uint16_t> cnt{};
    interval_event             a(
        [&]() {
            cnt++;
            std::cout << duration_cast<milliseconds>(system_clock::now() - start_time) << std::endl;
        },
        100ms, 1ms);
    std::this_thread::sleep_for(2s);
    auto cnt_val = cnt.load();
    std::cout << cnt_val << std::endl;
    EXPECT_TRUE((cnt_val >= 20) && (cnt_val <= 21));
}

TEST(interval_test, stop_check)
{
    auto                       start_time{system_clock::now()};
    std::atomic<std::uint16_t> cnt{};
    interval_event             a(
        [&]() {
            cnt++;
            std::cout << duration_cast<milliseconds>(system_clock::now() - start_time) << std::endl;
        },
        100ms, 1ms);
    std::this_thread::sleep_for(1s);
    auto cnt_val = cnt.load();
    std::cout << cnt_val << std::endl;
    a.stop();
    std::this_thread::sleep_for(2s);
    auto cnt_val_after = cnt.load();
    std::cout << "After stop: " << cnt_val << std::endl;
    EXPECT_TRUE((cnt_val_after - cnt_val) <= 1);
}

TEST(interval_test, interval_change_check)
{
    constexpr std::uint16_t    change_count = 10;
    auto                       start_time{system_clock::now()};
    std::atomic<std::uint16_t> cnt{};
    interval_event             a(
        [&]() {
            if (cnt++ >= change_count) {
                a.period(50ms);
            }
            std::cout << duration_cast<milliseconds>(system_clock::now() - start_time) << std::endl;
        },
        100ms, 1ms);
    std::this_thread::sleep_for(2s);
    auto cnt_val = cnt.load();
    std::cout << cnt_val << std::endl;
    EXPECT_TRUE((cnt_val >= 29) && (cnt_val <= 31));
}
