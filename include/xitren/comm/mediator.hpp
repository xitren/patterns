/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 03.03.2023
*/
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

    /**
     * @brief The unique id of the module
     */
    std::size_t const id_;

    /**
     * @brief The pointer to the manager object
     */
    manager* mediator_{};

    /**
     * @brief The virtual function that is called when a message is received
     *
     * @param id the id of the message sender
     * @param ptr the pointer to the message data
     * @param sz the size of the message data
     */
    virtual void
    receive(std::size_t id, std::uint8_t* ptr, std::size_t sz)
        = 0;

protected:
    /**
     * @brief The virtual function that is called to send a message
     *
     * @param ptr the pointer to the message data
     * @param sz the size of the message data
     */
    virtual void
    send(std::uint8_t* ptr, std::size_t sz)
        = 0;
};

class manager {
public:
    /**
     * @brief Construct a new manager object
     */
    manager() = default;

    /**
     * @brief Destroy the manager object
     */
    virtual ~manager() = default;

    /**
     * @brief Add a module to the manager
     *
     * @param c the module to add
     */
    virtual void
    add_module(module& c)
        = 0;

    /**
     * @brief Remove a module from the manager
     *
     * @param c the module to remove
     */
    virtual void
    remove_module(module& c)
        = 0;

    /**
     * @brief Distribute a message to all modules
     *
     * @param sender the module that sent the message
     * @param ptr the pointer to the message data
     * @param sz the size of the message data
     */
    virtual void
    distribute(module const& sender, std::uint8_t* ptr, std::size_t sz)
        = 0;

protected:
};
}    // namespace base

template <class Data>
class receiver_impl {
public:
    /**
     * @brief This function is called when data is received
     *
     * @param data the data that was received
     */
    virtual void
    data(Data const& data)
        = 0;

    /**
     * @brief Virtual destructor
     */
    virtual ~receiver_impl() = default;
};

template <class Sends, class... Recvs>
class module : public base::module, public receiver_impl<Recvs>... {
    using receiver_impl<Recvs>::data...;
    using size_type = std::size_t;

public:
    /**
     * @brief Constructs a new module object
     *
     * @param m the reference to the manager object
     */
    explicit module(base::manager& m) : base::module(typeid(Sends).name(), &m) { mediator_->add_module(*this); }

    /**
     * @brief Destroy the module object
     */
    ~module() override { mediator_->remove_module(*this); }

    /**
     * @brief Sends a message to other modules
     *
     * @param field the message data
     */
    void
    send(Sends field)
    {
        data_converter<Sends> cnv{};
        cnv.fields = field;
        send(cnv.pure.data(), cnv.pure.size());
    }

    /**
     * @brief This function is called when a message is received
     *
     * @param id the id of the message sender
     * @param ptr the pointer to the message data
     * @param sz the size of the message data
     */
    void
    receive([[maybe_unused]] std::size_t const id, [[maybe_unused]] std::uint8_t* ptr,
            [[maybe_unused]] std::size_t sz) override
    {
        if constexpr (sizeof...(Recvs) > 0)
            receiver_converter_helper<Recvs...>::receiver_converter(*this, id, ptr, sz);
    }

protected:
    /**
     * @brief Sends a message to other modules
     *
     * @param ptr the pointer to the message data
     * @param sz the size of the message data
     */
    void
    send(std::uint8_t* ptr, std::size_t sz) override
    {
        mediator_->distribute(*this, ptr, sz);
    }

private:
    /**
     * @brief Converts a message data to a specific type
     *
     * @param ptr the pointer to the message data
     * @param sz the size of the message data
     * @return true if the conversion is successful, false otherwise
     */
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

    /**
     * @brief A helper function to convert a received message
     *
     * @tparam First the first type of the message
     * @tparam Args the remaining types of the message
     * @param id the id of the message sender
     * @param ptr the pointer to the message data
     * @param sz the size of the message data
     */
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

    /**
     * @brief A helper function to convert a received message
     *
     * @tparam First the only type of the message
     * @param id the id of the message sender
     * @param ptr the pointer to the message data
     * @param sz the size of the message data
     */
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

    /**
     * @brief A union to hold a specific type of message
     *
     * @tparam T the type of the message
     */
    template <typename T>
    union data_converter {
        T                                   fields;
        std::array<std::uint8_t, sizeof(T)> pure;
    };
};

template <std::size_t Sources>
class manager : public base::manager {
public:
    /**
     * @brief Destroys the manager object
     */
    ~manager() override = default;

    /**
     * @brief Adds a module to the manager
     *
     * @param c the module to add
     */
    void
    add_module(base::module& c) override
    {
        modules_[modules_cnt_++] = &c;
    }

    /**
     * @brief Removes a module from the manager
     *
     * @param c the module to remove
     */
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

    /**
     * @brief Sends a message to other modules
     *
     * @param data the message data
     */
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

    /**
     * @brief This function is called when data is received
     *
     * @param id the id of the message sender
     * @param ptr the pointer to the message data
     * @param sz the size of the message data
     */
    virtual void
    data(std::size_t /*id*/, std::uint8_t* /*ptr*/, std::size_t /*sz*/)
    {}

    /**
     * @brief Distributes a message to all modules
     *
     * @param sender the module that sent the message
     * @param ptr the pointer to the message data
     * @param sz the size of the message data
     */
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

    /**
     * @brief Converts a message data to a specific type
     *
     * @param ptr the pointer to the message data
     * @param sz the size of the message data
     * @return T the converted message data
     */
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
    /**
     * @brief An array of pointers to the modules
     */
    std::array<base::module*, Sources> modules_;

    /**
     * @brief The number of modules in the manager
     */
    std::size_t modules_cnt_ = 0;

    /**
     * @brief A union to hold a specific type of message
     *
     * @tparam T the type of the message
     */
    template <typename T>
    union data_converter {
        T                                   fields;
        std::array<std::uint8_t, sizeof(T)> pure;
    };
};

}    // namespace xitren::comm
