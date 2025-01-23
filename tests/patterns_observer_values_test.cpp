#include <xitren/comm/values/observable.hpp>

#include <gtest/gtest.h>

using namespace xitren::comm::values;

TEST(observer_values, basic_two_observe)
{
    observable<int, 2> a{};
    observed<int>      b{};
    observed<int>      c{};

    a.add_observer(b);
    a.add_observer(c);
    a = 154;
    EXPECT_EQ(a.value(), b.value());
    EXPECT_EQ(a.value(), c.value());
}

TEST(observer_values, basic_two_observe_on_change)
{
    int fa{};
    int fb{};

    observable<int, 2> a{};
    observed<int>      b{[&](int val) {
        fa = 100;
        std::cout << val << std::endl;
    }};
    observed<int>      c{[&](int val) {
        fb = 101;
        std::cout << val << std::endl;
    }};

    a.add_observer(b);
    a.add_observer(c);
    a = 154;
    EXPECT_EQ(a.value(), b.value());
    EXPECT_EQ(a.value(), c.value());
    EXPECT_EQ(fa, 100);
    EXPECT_EQ(fb, 101);
}
