#include <xitren/comm/observer.hpp>

#include <gtest/gtest.h>

using namespace xitren::comm;

class test_observer : public observer<uint8_t> {
private:
    int i = 0;

public:
    void
    data(void const*, uint8_t const&) override
    {
        i = 1;
    }

    [[nodiscard]] int
    get() const
    {
        return i;
    }
};

class test_observer2 : public observer<uint8_t> {
private:
    int i = 0;

public:
    void
    data(void const*, uint8_t const&) override
    {
        i = 1;
    }

    [[nodiscard]] int
    get() const
    {
        return i;
    }
};

class test_observer_multi : public observer<uint8_t>, public observer<uint16_t> {
private:
    int i = 0;

public:
    void
    data(void const*, uint8_t const&) override
    {
        i = 1;
    }

    void
    data(void const*, uint16_t const&) override
    {
        i = 6;
    }

    [[nodiscard]] int
    get() const
    {
        return i;
    }
};

TEST(observer_test, basic_two_observe)
{
    test_observer              obs1;
    test_observer              obs2;
    observable<uint8_t, false> res1;
    res1.add_observer(obs1);
    res1.add_observer(obs2);

    uint8_t nd = {0};
    res1.notify_observers(nd);
    EXPECT_EQ(obs1.get(), 1);
}

TEST(observer_test, basic_add_observe)
{
    test_observer              obs1;
    observable<uint8_t, false> res1;
    res1.add_observer(obs1);

    uint8_t nd = {0};
    res1.notify_observers(nd);
    EXPECT_EQ(obs1.get(), 1);
}

TEST(observer_test, basic_add_observe_multi)
{
    test_observer              obs1;
    test_observer              obs2;
    test_observer              obs3;
    test_observer              obs4;
    observable<uint8_t, false> res1;
    res1.add_observer(obs1);
    res1.add_observer(obs2);
    res1.add_observer(obs3);
    res1.add_observer(obs4);

    uint8_t nd = {0};
    res1.notify_observers(nd);
    EXPECT_EQ(obs1.get(), 1);
    EXPECT_EQ(obs2.get(), 1);
    EXPECT_EQ(obs3.get(), 1);
    EXPECT_EQ(obs4.get(), 1);
}

TEST(observer_test, basic_add_observe_multi_diff)
{
    test_observer        obs1;
    test_observer        obs2;
    test_observer        obs3;
    test_observer_multi  obs4;
    observable<uint8_t>  res1;
    observable<uint16_t> res2;
    res1.add_observer(obs1);
    res1.add_observer(obs2);
    res1.add_observer(obs3);
    res1.add_observer(obs4);
    res2.add_observer(obs4);

    uint8_t  nd  = {0};
    uint16_t nd2 = {0};
    res1.remove_observer(obs4);
    res1.notify_observers(nd);
    res2.notify_observers(nd2);
    EXPECT_EQ(obs1.get(), 1);
    EXPECT_EQ(obs2.get(), 1);
    EXPECT_EQ(obs3.get(), 1);
    EXPECT_EQ(obs4.get(), 6);
}

class test_static_observer : public observer<uint8_t> {
private:
    int i = 0;

public:
    void
    data(void const*, uint8_t const&) override
    {
        i = 1;
    }

    [[nodiscard]] int
    get() const
    {
        return i;
    }
};

class test_static_observer2 : public observer<uint8_t> {
private:
    int i = 0;

public:
    void
    data(void const*, uint8_t const&) override
    {
        i = 1;
    }

    [[nodiscard]] int
    get() const
    {
        return i;
    }
};

class test_static_observer_multi : public observer<uint8_t>, public observer<uint16_t> {
private:
    int i = 0;

public:
    void
    data(void const*, uint8_t const&) override
    {
        i = 1;
    }

    void
    data(void const*, uint16_t const&) override
    {
        i = 6;
    }

    [[nodiscard]] int
    get() const
    {
        return i;
    }
};

TEST(connector_test, static_two_observe)
{
    test_static_observer          obs1;
    observable<uint8_t, true, 10> res1;
    res1.add_observer(obs1);

    uint8_t nd = {0};
    res1.notify_observers(nd);
    EXPECT_EQ(obs1.get(), 1);
}

TEST(connector_test, static_add_observe)
{
    test_static_observer          obs1;
    observable<uint8_t, true, 10> res1;
    res1.add_observer(obs1);

    uint8_t nd = {0};
    res1.notify_observers(nd);
    EXPECT_EQ(obs1.get(), 1);
}

TEST(connector_test, static_add_observe_multi)
{
    test_static_observer          obs1;
    test_static_observer          obs2;
    test_static_observer          obs3;
    test_static_observer          obs4;
    observable<uint8_t, true, 10> res1;
    res1.add_observer(obs1);
    res1.add_observer(obs2);
    res1.add_observer(obs3);
    res1.add_observer(obs4);

    uint8_t nd = {0};
    res1.notify_observers(nd);
    EXPECT_EQ(obs1.get(), 1);
    EXPECT_EQ(obs2.get(), 1);
    EXPECT_EQ(obs3.get(), 1);
    EXPECT_EQ(obs4.get(), 1);
}

TEST(connector_test, static_add_observe_multi_diff)
{
    test_static_observer           obs1;
    test_static_observer           obs2;
    test_static_observer           obs3;
    test_static_observer_multi     obs4;
    observable<uint8_t, true, 10>  res1;
    observable<uint16_t, true, 10> res2;
    res1.add_observer(obs1);
    res1.add_observer(obs2);
    res1.add_observer(obs3);
    res1.add_observer(obs4);
    res2.add_observer(obs4);

    uint8_t  nd  = {0};
    uint16_t nd2 = {0};
    res1.remove_observer(obs4);
    res1.notify_observers(nd);
    res2.notify_observers(nd2);
    EXPECT_EQ(obs1.get(), 1);
    EXPECT_EQ(obs2.get(), 1);
    EXPECT_EQ(obs3.get(), 1);
    EXPECT_EQ(obs4.get(), 6);
}
