/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 25.11.2024
*/
#pragma once

#include <cstdint>
#include <exception>

namespace xitren::cache {

class cache_timeout : public std::exception {
public:
    cache_timeout() : std::exception() {}

    char const*
    what() const noexcept override
    {
        static char const* problem = "Data in cache has expired!";
        return problem;
    }
};

class cache_missed : public std::exception {
public:
    cache_missed() : std::exception() {}

    char const*
    what() const noexcept override
    {
        static char const* problem = "Cache does not contains key!";
        return problem;
    }
};

}    // namespace xitren::cache
