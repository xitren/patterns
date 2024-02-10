#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <typeinfo>
#include <utility>
#include <variant>

namespace xitren::comm {

namespace base {
class manager;

class module {
public:
    explicit module(std::string const& id, manager* m = nullptr) : id_(std::hash<std::string>{}(id)), mediator_(m) {}

    virtual ~module() = default;

    std::size_t const id_;
    manager*          mediator_{};

    virtual void
    receive(std::size_t id, std::uint8_t*, std::size_t)
        = 0;

protected:
    virtual void
    send(std::uint8_t*, std::size_t)
        = 0;
};

class manager {
public:
    manager()          = default;
    virtual ~manager() = default;

    virtual void
    add_module(module& c)
        = 0;
    virtual void
    remove_module(module& c)
        = 0;
    virtual void
    distribute(module const& sender, std::uint8_t*, std::size_t)
        = 0;

protected:
};
}    // namespace base

template <class Data>
class receiver_impl {
public:
    virtual void
    data(Data const& data)
        = 0;
    virtual ~receiver_impl() = default;
};

template <class Sends, class... Recvs>
class module : public base::module, public receiver_impl<Recvs>... {
    using receiver_impl<Recvs>::data...;
    using size_type = std::size_t;

public:
    explicit module(base::manager& m) : base::module(typeid(Sends).name(), &m) { mediator_->add_module(*this); }
    ~module() override { mediator_->remove_module(*this); }
    void
    send(Sends field)
    {
        data_converter<Sends> cnv{};
        cnv.fields = field;
        send(cnv.pure.data(), cnv.pure.size());
    }

    void
    receive([[maybe_unused]] std::size_t const id, [[maybe_unused]] std::uint8_t* ptr,
            [[maybe_unused]] std::size_t sz) override
    {
        if constexpr (sizeof...(Recvs) > 0)
            receiver_converter_helper<Recvs...>::receiver_converter(*this, id, ptr, sz);
    }

protected:
    void
    send(std::uint8_t* ptr, std::size_t sz) override
    {
        mediator_->distribute(*this, ptr, sz);
    }

private:
    template <class T>
    constexpr bool
    convert_field(std::uint8_t* ptr, std::size_t sz)
    {
        data_converter<T> cnv = {};
        if (cnv.pure.size() != sz)
            return false;
        for (auto& item : cnv.pure) {
            item = *(ptr++);
        }
        this->data(cnv.fields);
        return true;
    }

    template <typename First, typename... Args>
    struct receiver_converter_helper {
        static constexpr void
        receiver_converter(module<Sends, Recvs...>& val, std::size_t id, std::uint8_t* ptr, std::size_t sz)
        {
            std::size_t const hash = std::hash<std::string>{}(typeid(First).name());
            if (id == hash) {
                val.convert_field<First>(ptr, sz);
                return;
            } else {
                return receiver_converter_helper<Args...>::receiver_converter(val, id, ptr, sz);
            }
        }
    };
    template <typename First>
    struct receiver_converter_helper<First> {
        static constexpr void
        receiver_converter(module<Sends, Recvs...>& val, std::size_t id, std::uint8_t* ptr, std::size_t sz)
        {
            std::size_t const hash = std::hash<std::string>{}(typeid(First).name());
            if (id == hash) {
                val.convert_field<First>(ptr, sz);
                return;
            } else {
                return;
            }
        }
    };

    template <typename T>
    union data_converter {
        T                                   fields;
        std::array<std::uint8_t, sizeof(T)> pure;
    };
};

template <std::size_t Sources>
class manager : public base::manager {
public:
    ~manager() override = default;

    void
    add_module(base::module& c) override
    {
        modules_[modules_cnt_++] = &c;
    }

    void
    remove_module(base::module& c) override
    {
        for (std::size_t i = 0; i < modules_cnt_; i++) {
            if (modules_[i] == &c) {
                for (; i < modules_cnt_ - 1; i++) {
                    modules_[i] = modules_[i + 1];
                }
                modules_[modules_cnt_] = 0;
                --modules_cnt_;
                return;
            }
        }
    }

    template <class Data>
    void
    send(Data data)
    {
        for (std::size_t i = 0; i < modules_cnt_; i++) {
            std::size_t          loc_id = std::hash<std::string>{}(typeid(Data).name());
            data_converter<Data> cnv    = {data};
            if (modules_.at(i)->id_ != loc_id) {
                modules_.at(i)->receive(loc_id, cnv.pure.data(), cnv.pure.size());
            }
        }
    }

    virtual void
    data(std::size_t /*id*/, std::uint8_t* /*ptr*/, std::size_t /*sz*/)
    {}

    void
    distribute(base::module const& sender, std::uint8_t* ptr, std::size_t sz) override
    {
        data(sender.id_, ptr, sz);
        for (std::size_t i = 0; i < modules_cnt_; i++) {
            if (modules_.at(i)->id_ != sender.id_) {
                modules_.at(i)->receive(sender.id_, ptr, sz);
            }
        }
    }

    template <class T>
    constexpr T
    convert_field(std::uint8_t* ptr, std::size_t sz)
    {
        data_converter<T> cnv = {};
        if (cnv.pure.size() != sz) {
            throw std::exception();
        }
        for (auto& item : cnv.pure) {
            item = *(ptr++);
        }
        return cnv.fields;
    }

private:
    std::array<base::module*, Sources> modules_;
    std::size_t                        modules_cnt_ = 0;

    template <typename T>
    union data_converter {
        T                                   fields;
        std::array<std::uint8_t, sizeof(T)> pure;
    };
};

}    // namespace xitren::comm
