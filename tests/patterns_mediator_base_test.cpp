#include <xitren/comm/mediator.hpp>

#include <gtest/gtest.h>

#include <cstring>
#include <iostream>

using namespace xitren::comm;

class data1 {
public:
    int i1;
};
class data2 {
public:
    int i1;
    int i2;
};
class data3 {
public:
    int i1;
    int i2;
};

class m1 : module<data1, data2> {
public:
    explicit m1(base::manager& m) : module(m) {}

    void
    data(data2 const&) override
    {
        std::cout << "Receiving m1!" << std::endl;
    }

    void
    test()
    {
        data1 a{};
        send(a);
    }
};

class m2 : module<data2, data3> {
public:
    explicit m2(base::manager& m) : module(m) {}

    void
    data(data3 const&) override
    {
        std::cout << "Receiving m2!" << std::endl;
    }

    void
    test()
    {
        data2 a{};
        send(a);
    }
};

class m3 : module<data3, data2, data1> {
public:
    explicit m3(base::manager& m) : module(m) {}

    void
    data(data2 const&) override
    {
        std::cout << "Receiving m3 data2!" << std::endl;
    }

    void
    data(data1 const&) override
    {
        std::cout << "Receiving m3 data1!" << std::endl;
    }
};

TEST(mediator_test, basic)
{
    manager<5> dt;
    m1         mod1(dt);
    m2         mod2(dt);
    m3         mod3(dt);

    std::cout << "Sending event!" << std::endl;
    mod2.test();
    mod1.test();

    dt.send(data3{});
}
