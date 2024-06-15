#include <xitren/func/argv_parser.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <iostream>

using namespace xitren::func;

TEST(argv_test, usual)
{
    char const* argv[] = {"app", "--port", "/dev/tty.usbserial-A50285BI", "--baud", "1000000"};
    int         argc   = sizeof(argv) / sizeof(argv[0]);

    struct options {
        std::string name_port{};
        int         baud_rate{};
    };

    auto parser = argv_parser<options>::instance({{"--port", &options::name_port}, {"--baud", &options::baud_rate}});

    auto detected_opts = parser->parse(argc, argv);

    std::cout << "name_port = " << detected_opts.name_port << std::endl;
    std::cout << "baud_rate = " << detected_opts.baud_rate << std::endl;
    EXPECT_TRUE(detected_opts.name_port == "/dev/tty.usbserial-A50285BI");
    EXPECT_TRUE(detected_opts.baud_rate == 1000000);
}

TEST(argv_test, help)
{
    char const* argv[] = {"app", "--help"};
    int         argc   = sizeof(argv) / sizeof(argv[0]);

    struct options {
        bool help{false};
    };

    auto parser = argv_parser<options>::instance({{"--help", &options::help}});

    auto detected_opts = parser->parse(argc, argv);

    std::cout << "help = " << detected_opts.help << std::endl;
    EXPECT_TRUE(detected_opts.help);
}

TEST(argv_test, not_help)
{
    char const* argv[] = {"app"};
    int         argc   = sizeof(argv) / sizeof(argv[0]);

    struct options {
        bool help{false};
    };

    auto parser = argv_parser<options>::instance({{"--help", &options::help}});

    auto detected_opts = parser->parse(argc, argv);

    std::cout << "help = " << detected_opts.help << std::endl;
    EXPECT_TRUE(!detected_opts.help);
}
