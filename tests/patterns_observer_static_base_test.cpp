#include <xitren/comm/observer_static.hpp>

#include <gtest/gtest.h>

using namespace xitren::comm;

struct some_data {};

struct observer1 : public observer<some_data, observer1> {
    int count_ = 0;
    inline void
    notification_impl(observable_impl<some_data> const& /*src*/, some_data const& /*data*/) noexcept
    {
        count_++;
    }
};
struct observer2 : observer<some_data, observer2> {
    int count_ = 0;
    inline void
    notification_impl(observable_impl<some_data> const& /*src*/, some_data const& /*data*/) noexcept
    {
        count_++;
    }
};
struct observer3 : observer<some_data, observer3> {
    int count_ = 0;
    inline void
    notification_impl(observable_impl<some_data> const& /*src*/, some_data const& /*data*/) noexcept
    {
        count_++;
    }
};

struct observable_l : public observable<some_data, observer1, observer2, observer3> {};

TEST(observer_static_test, basic_three_observe)
{
    observer1                                              ob1;
    observer2                                              ob2;
    observer3                                              ob3;
    observable<some_data, observer1, observer2, observer3> a{ob1, ob2, ob3};

    some_data b;
    a.notify(b);
    EXPECT_EQ(ob1.count_, 1);
    EXPECT_EQ(ob2.count_, 1);
    EXPECT_EQ(ob3.count_, 1);
}

TEST(observer_static_test, basic_three_observe_multi_diff)
{
    observer1                                              ob1;
    observer2                                              ob2;
    observer3                                              ob3;
    observable<some_data, observer1, observer2, observer3> a1{ob1, ob2, ob3};
    observable<some_data, observer3>                       a2{ob3};

    some_data b;
    a1.notify(b);
    some_data c;
    a2.notify(c);
    EXPECT_EQ(ob1.count_, 1);
    EXPECT_EQ(ob2.count_, 1);
    EXPECT_EQ(ob3.count_, 2);
}
