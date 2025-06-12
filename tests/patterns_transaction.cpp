#include <xitren/func/transaction.hpp>

#include <gtest/gtest.h>

using namespace xitren::func;

TEST(transaction, positive)
{
    int data1 = 85;
    int data2 = 68;

    try {
        auto trn = make_transaction(data1, data2);

        data1 = 102;
        data2 = 59;

        // ...

        trn.commit();

    } catch (std::exception const&) {}

    EXPECT_TRUE(data1 == 102);
    EXPECT_TRUE(data2 == 59);
}

TEST(transaction, negative)
{
    int data1 = 85;
    int data2 = 68;

    try {
        auto trn = make_transaction(data1, data2);

        data1 = 102;
        data2 = 59;

        // ...

        throw std::runtime_error("Problem!");

        trn.commit();

    } catch (std::exception const&) {}

    EXPECT_TRUE(data1 == 85);
    EXPECT_TRUE(data2 == 68);
}
