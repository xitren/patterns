#include <xitren/comm/pipeline_stage.hpp>

#include <gtest/gtest.h>

#include <iostream>

using namespace xitren::comm;

struct LogCout {
    static auto&
    trace()
    {
        return std::cout;
    }
    static auto&
    debug()
    {
        return std::cout;
    }
    static auto&
    warning()
    {
        return std::cout;
    }
    static auto&
    error()
    {
        return std::cerr;
    }
    static auto&
    critical()
    {
        return std::cerr;
    }
};

TEST(pipeline_test, basic)
{
    using pipeline_type = pipeline_stage<std::string, std::string, 1024, LogCout>;
    std::atomic<int> i{};
    {
        pipeline_type stage([&](pipeline_stage_exception ex, const std::string str) -> const std::string {
            LogCout::debug() << "String: " << str << "\n";
            i++;
            return str;
        });
        stage.push("First");
        stage.push("Second");
        stage.push("Third");
        LogCout::debug() << std::endl;
    }
    EXPECT_EQ(i, 3);
}
