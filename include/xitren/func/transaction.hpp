/*!
     _ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 13.06.2025
*/
#pragma once

#include <tuple>

namespace xitren::func {

template <typename... Args>
class transaction {
public:
    transaction(Args&... args) : refs_(args...), vals_(args...) {}

    ~transaction()
    {
        if (incomplete_) {
            revert();
        }
    }

    void
    commit()
    {
        incomplete_ = false;
    }

    // Disable copy and allow move
    transaction(transaction const&) = delete;
    transaction&
    operator=(transaction const&)
        = delete;
    transaction(transaction&&) noexcept = default;
    transaction&
    operator=(transaction&&) noexcept
        = default;

private:
    void
    revert()
    {
        refs_ = vals_;
    }

    std::tuple<Args&...> refs_;
    std::tuple<Args...>  vals_;
    bool                 incomplete_ = true;
};

template <typename... Args>
transaction<Args...>
make_transaction(Args&... args)
{
    return transaction<Args...>(args...);
}

}    // namespace xitren::func
