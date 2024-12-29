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
    static auto      func
        = [&](pipeline_stage_exception ex, const std::string str, const std::pair<int, int> stat) -> const std::string {
        LogCout::debug() << "String: " << str << "\n";
        i++;
        return str;
    };
    {
        pipeline_type stage(func);
        stage.push("First");
        stage.push("Second");
        stage.push("Third");
        LogCout::debug() << std::endl;
    }
    EXPECT_EQ(i, 3);
}

TEST(pipeline_test, basic_void_ret)
{
    using pipeline_type = pipeline_stage<std::string, void, 1024, LogCout>;
    std::atomic<int> i{};
    static auto func = [&](pipeline_stage_exception ex, const std::string str, const std::pair<int, int> stat) -> void {
        LogCout::debug() << "String: " << str << " Time: " << stat.first << " Util: " << stat.second << "\n";
        using namespace std::chrono;
        i++;
    };
    {
        pipeline_type stage(func);
        stage.push("First");
        stage.push("Second");
        stage.push("Third");
        for (int i{}; i < 100; i++) {
            stage.push("Next");
            stage.push("Other");
        }
        LogCout::debug() << std::endl;
    }
    EXPECT_EQ(i, 203);
}
