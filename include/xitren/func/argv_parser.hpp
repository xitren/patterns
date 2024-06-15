#pragma once

#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace xitren::func {

template <class Opts>
struct argv_parser : Opts {
    using parameter_type = std::variant<std::string Opts::*, int Opts::*, double Opts::*, bool Opts::*>;
    using argument_type  = std::pair<std::string, parameter_type>;

    ~argv_parser() = default;

    argv_parser(argv_parser const&) = delete;

    argv_parser(argv_parser&&) = delete;

    argv_parser&
    operator=(argv_parser const&)
        = delete;

    argv_parser&
    operator=(argv_parser&&)
        = delete;

    Opts
    parse(int argc, char const* argv[])
    {
        std::vector<std::string_view> vargv(argv, argv + argc);
        for (int idx = 0; idx < argc; ++idx)
            for (auto& cbk : callbacks_)
                cbk.second(idx, vargv);
        return static_cast<Opts>(*this);
    }

    static std::unique_ptr<argv_parser>
    instance(std::initializer_list<argument_type> args)
    {
        auto cmd_opts = std::unique_ptr<argv_parser>(new argv_parser());
        for (auto arg : args)
            cmd_opts->register_callback(arg);
        return cmd_opts;
    }

private:
    using callback_t = std::function<void(int, std::vector<std::string_view> const&)>;
    std::map<std::string, callback_t> callbacks_;

    argv_parser() = default;

    auto
    register_callback(argument_type p)
    {
        return register_callback(p.first, p.second);
    }

    auto
    register_callback(std::string const& name, parameter_type prop)
    {
        callbacks_[name] = [this, name, prop](int idx, std::vector<std::string_view> const& argv) {
            if (argv[idx] == name) {
                if (prop.index() == 3) {
                    this->*std::get<3>(prop) = true;
                    return;
                }
                visit(
                    [this, idx, &argv](auto&& arg) {
                        if (idx < static_cast<int>(argv.size() - 1)) {
                            std::stringstream value;
                            value << argv[idx + 1];
                            value >> this->*arg;
                        }
                    },
                    prop);
            }
        };
    };
};

}    // namespace xitren::func