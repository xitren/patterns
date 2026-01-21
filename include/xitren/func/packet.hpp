/*!
     _ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 03.03.2024
*/
#pragma once

#include <xitren/func/data.hpp>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <tuple>
#include <utility>

namespace xitren::func {

template <typename T>
concept crc_concept = requires {
    typename T::value_type;
};

template <typename Header, typename Fields, crc_concept Crc>
class packet {
    using size_type                   = std::size_t;
    using crc_type                    = typename Crc::value_type;
    static constexpr size_type length = (sizeof(Header) + sizeof(Fields) + sizeof(crc_type));

    using array_type = std::array<std::uint8_t, length>;

public:
    explicit constexpr packet(array_type const& array) noexcept : bytes_{array} {}
    explicit constexpr packet(array_type&& array) noexcept : bytes_{std::move(array)} {}

    template <std::input_iterator InputIterator>
    explicit constexpr packet(InputIterator begin) noexcept : bytes_{}
    {
        for (size_type i{}; i < length; ++i, ++begin) {
            bytes_[i] = static_cast<std::uint8_t>(*begin);
        }
    }

    constexpr packet() noexcept : bytes_{} {}

    constexpr packet(Header const& header, Fields const& fields) noexcept : bytes_{serialize(header, fields)}
    {
    }

    [[nodiscard]] constexpr Header
    header() const noexcept
    {
        return data<Header>::deserialize(bytes_.begin());
    }

    [[nodiscard]] constexpr Fields
    fields() const noexcept
    {
        return data<Fields>::deserialize(bytes_.begin() + sizeof(Header));
    }

    [[nodiscard]] constexpr crc_type
    crc() const noexcept
    {
        return data<crc_type>::deserialize(bytes_.begin() + sizeof(Header) + sizeof(Fields));
    }

    constexpr bool
    valid() const noexcept
    {
        auto const calc = Crc::calculate(bytes_.begin(), bytes_.begin() + sizeof(Header) + sizeof(Fields));
        return crc() == calc;
    }

    constexpr array_type
    to_array() const noexcept
    {
        return bytes_;
    }

    static constexpr array_type
    serialize(Header const& header, Fields const& fields) noexcept
    {
        array_type out{};
        data<Header>::serialize(header, out.begin());
        data<Fields>::serialize(fields, out.begin() + sizeof(Header));
        auto const crc = Crc::calculate(out.begin(), out.begin() + sizeof(Header) + sizeof(Fields));
        data<crc_type>::serialize(crc, out.begin() + sizeof(Header) + sizeof(Fields));
        return out;
    }

    template <std::size_t Size>
    static constexpr std::tuple<bool, Header, Fields>
    deserialize(std::array<std::uint8_t, Size> const& array) noexcept
    {
        static_assert(Size >= length);
        auto const header_v = data<Header>::deserialize(array.begin());
        auto const fields_v = data<Fields>::deserialize(array.begin() + sizeof(Header));
        auto const crc_v    = data<crc_type>::deserialize(array.begin() + sizeof(Header) + sizeof(Fields));
        auto const calc     = Crc::calculate(array.begin(), array.begin() + sizeof(Header) + sizeof(Fields));
        return {crc_v == calc, header_v, fields_v};
    }

    template <typename InputIterator>
    static constexpr packet
    deserialize(InputIterator begin) noexcept
    {
        return packet{begin};
    }

    template <std::output_iterator<std::uint8_t> OutputIterator>
    static constexpr void
    serialize(packet const& type, OutputIterator begin) noexcept
    {
        auto const bytes = type.to_array();
        for (auto b : bytes) {
            *begin++ = b;
        }
    }

private:
    array_type bytes_;
};

template <std::size_t Max>
class packet_accessor {
    using size_type = std::size_t;

public:
    using array_type = std::array<std::uint8_t, Max>;

    template <typename Header, typename Fields, typename Type>
    struct __attribute__((__packed__)) fields_out {
        Header      header;
        Fields      fields;
        bool        valid;
        size_type   size;
        Type const* data;
    };

    template <typename Header, typename Fields, typename Type>
    struct __attribute__((__packed__)) fields_out_ptr {
        Header const* const header;
        Fields const* const fields;
        size_type           size;
        Type const*         data;
    };

    template <typename Header, typename Fields, typename Type>
    struct __attribute__((__packed__)) fields_in {
        Header      header;
        Fields      fields;
        size_type   size;
        Type const* data;
    };

    template <typename Header, typename Fields, typename Type, crc_concept Crc>
    auto
    deserialize_no_check() const noexcept
    {
        using return_type          = fields_out_ptr<Header, Fields, Type>;
        constexpr size_type length = (sizeof(Header) + sizeof(Fields) + sizeof(typename Crc::value_type));
        static_assert(sizeof(Type) != 0);
        static_assert(Max >= length);
        size_type const variable_part = (size_ - length) / sizeof(Type);
        auto header_conv = reinterpret_cast<Header const*>(storage_.data());
        auto fields_conv = reinterpret_cast<Fields const*>(storage_.data() + sizeof(Header));
        auto data_conv   = reinterpret_cast<Type const*>(storage_.data() + sizeof(Header) + sizeof(Fields));
        return return_type{header_conv, fields_conv, variable_part, data_conv};
    }

    template <typename Header, typename Fields, typename Type, crc_concept Crc>
    constexpr auto
    deserialize() const
    {
        using return_type          = fields_out<Header, Fields, Type>;
        constexpr size_type length = (sizeof(Header) + sizeof(Fields) + sizeof(typename Crc::value_type));
        static_assert(sizeof(Type) != 0);
        static_assert(Max >= length);
        size_type const variable_part = (size_ - length) / sizeof(Type);
        if (((size_ - length) % sizeof(Type))) {
            return return_type{{}, {}, false, 0, nullptr};
        }
        auto header_conv = data<Header>::deserialize(storage_.begin());
        auto fields_conv = data<Fields>::deserialize(storage_.begin() + sizeof(Header));
        auto crc_conv
            = data<typename Crc::value_type>::deserialize(storage_.begin() + size_ - sizeof(typename Crc::value_type));
        typename Crc::value_type crc_calc
            = Crc::calculate(storage_.begin(), storage_.begin() + size_ - sizeof(typename Crc::value_type));
        return return_type{header_conv, fields_conv, crc_conv.get() == crc_calc.get(), variable_part,
                           reinterpret_cast<Type const*>(storage_.begin() + sizeof(Header) + sizeof(Fields))};
    }

    template <typename Header, typename Fields, typename Type, crc_concept Crc>
    constexpr bool
    serialize(fields_in<Header, Fields, Type> const& input)
    {
        constexpr size_type length = (sizeof(Header) + sizeof(Fields) + sizeof(typename Crc::value_type));
        static_assert(sizeof(Type) != 0);
        static_assert(Max >= length);
        if ((input.size * sizeof(Type) + length) > Max) {
            return false;
        }
        data<Header>::serialize(input.header, storage_.begin());
        data<Fields>::serialize(input.fields, storage_.begin() + sizeof(Header));
        if ((input.size > 0) && (input.data != nullptr)) {
            std::copy(reinterpret_cast<uint8_t const*>(input.data),
                      reinterpret_cast<uint8_t const*>(input.data + input.size),
                      reinterpret_cast<uint8_t*>(storage_.begin() + sizeof(Header) + sizeof(Fields)));
        }
        auto const crc_ptr = storage_.begin() + sizeof(Header) + sizeof(Fields) + input.size * sizeof(Type);
        typename Crc::value_type const crc{Crc::calculate(storage_.begin(), crc_ptr)};
        data<typename Crc::value_type>::serialize(crc, crc_ptr);
        size_ = length + input.size * sizeof(Type);
        return true;
    }

    [[nodiscard]] size_type
    size() const
    {
        return size_;
    }

    void
    size(size_type size)
    {
        size_ = size;
    }

    inline array_type&
    storage() noexcept
    {
        return storage_;
    }

    inline array_type const&
    storage() const noexcept
    {
        return storage_;
    }

private:
    array_type storage_{};
    size_type  size_{};
};

}    // namespace xitren::func
