#pragma once

#include <charconv>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
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
        for (int idx = 0; idx < argc; ++idx) {
            auto it = args_.find(std::string(vargv[idx]));
            if (it == args_.end()) {
                continue;
            }
            auto const& prop = it->second;
            if (prop.index() == 3) {
                this->*std::get<3>(prop) = true;
                continue;
            }
            if (idx >= static_cast<int>(vargv.size() - 1)) {
                continue;
            }
            auto const val = vargv[idx + 1];
            std::visit(
                [this, val](auto&& member) {
                    using member_t = std::decay_t<decltype(member)>;
                    if constexpr (std::is_same_v<member_t, std::string Opts::*>) {
                        this->*member = std::string(val);
                    } else if constexpr (std::is_same_v<member_t, int Opts::*>) {
                        int out{};
                        auto [p, ec] = std::from_chars(val.data(), val.data() + val.size(), out);
                        if (ec == std::errc{} && p == val.data() + val.size()) {
                            this->*member = out;
                        }
                    } else if constexpr (std::is_same_v<member_t, double Opts::*>) {
                        double out{};
                        auto [p, ec] = std::from_chars(val.data(), val.data() + val.size(), out);
                        if (ec == std::errc{} && p == val.data() + val.size()) {
                            this->*member = out;
                        } else {
                            // Fallback (slower, but portable)
                            try {
                                this->*member = std::stod(std::string(val));
                            } catch (...) {
                            }
                        }
                    } else {
                        // bool handled above
                    }
                },
                prop);
        }
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
    std::unordered_map<std::string, parameter_type> args_;

    argv_parser() = default;

    auto
    register_callback(argument_type p)
    {
        return register_callback(p.first, p.second);
    }

    auto
    register_callback(std::string const& name, parameter_type prop)
    {
        args_[name] = prop;
    };
};

}    // namespace xitren::func