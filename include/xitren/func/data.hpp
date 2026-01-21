/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 03.03.2023
*/
#pragma once

#include <array>
#include <concepts>
#include <cstdint>
#include <cstring>
#if defined(__cpp_lib_endian)
#include <bit>
#endif
#include <type_traits>
#include <utility>

namespace xitren::func {

static constexpr bool
is_lsb() noexcept
{
#if defined(__cpp_lib_endian)
    return std::endian::native == std::endian::little;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
    return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#elif defined(_WIN32) || defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__AARCH64EL__) || defined(__MIPSEL__) \
    || defined(__i386__) || defined(__x86_64__)
    return true;
#else
    // Conservative fallback: unknown target => assume big-endian is possible.
    return false;
#endif
}

template <class Type>
struct data {
    static_assert(std::is_trivially_copyable_v<Type>,
                  "xitren::func::data requires trivially copyable Type (for safe byte serialization)");

    using value_type = Type;
    static constexpr std::size_t size = sizeof(Type);
    using array_type                  = std::array<std::uint8_t, size>;

    template <std::input_iterator InputIterator>
    static constexpr Type
    deserialize(InputIterator begin) noexcept
    {
        Type      out{};
        array_type tmp{};
        for (std::size_t i{}; i < size; ++i, ++begin) {
            tmp[i] = static_cast<std::uint8_t>(*begin);
        }
        std::memcpy(&out, tmp.data(), size);
        return out;
    }

    static constexpr array_type
    serialize(Type const& value) noexcept
    {
        array_type out{};
        std::memcpy(out.data(), &value, size);
        return out;
    }

    template <std::output_iterator<std::uint8_t> OutputIterator>
    static constexpr void
    serialize(Type const& value, OutputIterator begin) noexcept
    {
        auto const bytes = serialize(value);
        for (auto b : bytes) {
            *begin++ = b;
        }
    }
};

/**
 * swaps the bytes of a 16-bit unsigned integer
 * @param val the value to be swapped
 * @return the swapped value
 */
static constexpr std::uint16_t
swap(std::uint16_t val) noexcept
{
    auto bytes = data<std::uint16_t>::serialize(val);
    std::swap(bytes[0], bytes[1]);
    return data<std::uint16_t>::deserialize(bytes.begin());
}

/**
 * swaps the bytes of a 32-bit unsigned integer
 * @param val the value to be swapped
 * @return the swapped value
 */
static constexpr std::uint32_t
swap(std::uint32_t val) noexcept
{
    auto bytes = data<std::uint32_t>::serialize(val);
    std::swap(bytes[0], bytes[3]);
    std::swap(bytes[1], bytes[2]);
    return data<std::uint32_t>::deserialize(bytes.begin());
}

/**
 * swaps the bytes of a 64-bit unsigned integer
 * @param val the value to be swapped
 * @return the swapped value
 */
static constexpr std::uint64_t
swap(std::uint64_t val) noexcept
{
    auto bytes = data<std::uint64_t>::serialize(val);
    std::swap(bytes[0], bytes[7]);
    std::swap(bytes[1], bytes[6]);
    std::swap(bytes[2], bytes[5]);
    std::swap(bytes[3], bytes[4]);
    return data<std::uint64_t>::deserialize(bytes.begin());
}

template <class T>
concept swappable = std::same_as<std::uint16_t, T> || std::same_as<std::uint32_t, T> || std::same_as<std::uint64_t, T>;

template <swappable T>
class __attribute__((__packed__)) lsb_t {
public:
    /**
     * The underlying data type of the lsb_t.
     */
    using data_type = T;

    /**
     * Default constructor.
     */
    constexpr lsb_t() = default;

    /**
     * Constructs a lsb_t from a given value.
     *
     * If the current CPU is little endian, the value is stored in the least significant bytes. Otherwise, it is stored
     * in the most significant bytes.
     *
     * @param value The value to be stored in the lsb_t.
     */
    constexpr lsb_t(data_type const& value)
    {
        if constexpr (is_lsb()) {
            value_ = value;
        } else {
            value_ = swap(value);
        }
    }

    /**
     * Constructs a lsb_t from a given rvalue.
     *
     * If the current CPU is little endian, the value is stored in the least significant bytes. Otherwise, it is stored
     * in the most significant bytes.
     *
     * @param value The rvalue to be stored in the lsb_t.
     */
    constexpr lsb_t(data_type const&& value)
    {
        if constexpr (is_lsb()) {
            value_ = value;
        } else {
            value_ = swap(value);
        }
    }

    /**
     * Returns the value of the lsb_t.
     *
     * If the current CPU is little endian, the value is returned in the least significant bytes. Otherwise, it is
     * returned in the most significant bytes.
     *
     * @return The value of the lsb_t.
     */
    [[nodiscard]] constexpr data_type
    get() const
    {
        if constexpr (is_lsb()) {
            return value_;
        } else {
            return swap(value_);
        }
    }

    /**
     * Assigns a new value to the lsb_t.
     *
     * If the current CPU is little endian, the new value is stored in the least significant bytes. Otherwise, it is
     * stored in the most significant bytes.
     *
     * @param value The new value to be assigned to the lsb_t.
     * @return A reference to the lsb_t.
     */
    constexpr lsb_t&
    operator=(data_type value)
    {
        if constexpr (is_lsb()) {
            value_ = value;
        } else {
            value_ = swap(value);
        }
        return *this;
    }

    /**
     * Compares two lsb_ts for equality.
     *
     * @param other The lsb_t to be compared with.
     * @return True if the two lsb_ts are equal, false otherwise.
     */
    constexpr bool
    operator==(lsb_t const& other)
    {
        return this->get() == other.get();
    }

    /**
     * Compares two lsb_ts for less-than.
     *
     * @param other The lsb_t to be compared with.
     * @return True if the value of this lsb_t is less than the value of the other lsb_t, false otherwise.
     */
    constexpr bool
    operator<(lsb_t const& other)
    {
        return this->get() < other.get();
    }

    /**
     * Compares two lsb_ts for greater-than.
     *
     * @param other The lsb_t to be compared with.
     * @return True if the value of this lsb_t is greater than the value of the other lsb_t, false otherwise.
     */
    constexpr bool
    operator>(lsb_t const& other)
    {
        return this->get() > other.get();
    }

    /**
     * Compares two lsb_ts for less-than-or-equal.
     *
     * @param other The lsb_t to be compared with.
     * @return True if the value of this lsb_t is less than or equal to the value of the other lsb_t, false otherwise.
     */
    constexpr bool
    operator<=(lsb_t const& other)
    {
        return this->get() <= other.get();
    }

    /**
     * Compares two lsb_ts for greater-than-or-equal.
     *
     * @param other The lsb_t to be compared with.
     * @return True if the value of this lsb_t is greater than or equal to the value of the other lsb_t, false
     * otherwise.
     */
    constexpr bool
    operator>=(lsb_t const& other)
    {
        return this->get() >= other.get();
    }

private:
    /**
     * The actual value stored in the lsb_t.
     */
    data_type value_{};
};

template <swappable T>
class __attribute__((__packed__)) msb_t {
public:
    /**
     * The underlying data type of the msb_t.
     */
    using data_type = T;

    /**
     * Default constructor.
     */
    constexpr msb_t() = default;

    /**
     * Constructs a msb_t from a given value.
     *
     * If the current CPU is little endian, the value is stored in the most significant bytes. Otherwise, it is stored
     * in the least significant bytes.
     *
     * @param value The value to be stored in the msb_t.
     */
    constexpr msb_t(data_type const& value)
    {
        if constexpr (is_lsb()) {
            value_ = swap(value);
        } else {
            value_ = value;
        }
    }

    /**
     * Constructs a msb_t from a given rvalue.
     *
     * If the current CPU is little endian, the value is stored in the most significant bytes. Otherwise, it is stored
     * in the least significant bytes.
     *
     * @param value The rvalue to be stored in the msb_t.
     */
    constexpr msb_t(data_type const&& value)
    {
        if constexpr (is_lsb()) {
            value_ = swap(value);
        } else {
            value_ = value;
        }
    }

    /**
     * Returns the value of the msb_t.
     *
     * If the current CPU is little endian, the value is returned in the most significant bytes. Otherwise, it is
     * returned in the least significant bytes.
     *
     * @return The value of the msb_t.
     */
    [[nodiscard]] constexpr data_type
    get() const
    {
        if constexpr (is_lsb()) {
            return swap(value_);
        } else {
            return value_;
        }
    }

    /**
     * Assigns a new value to the msb_t.
     *
     * If the current CPU is little endian, the new value is stored in the most significant bytes. Otherwise, it is
     * stored in the least significant bytes.
     *
     * @param value The new value to be assigned to the msb_t.
     * @return A reference to the msb_t.
     */
    constexpr msb_t&
    operator=(data_type value)
    {
        if constexpr (is_lsb()) {
            value_ = swap(value);
        } else {
            value_ = value;
        }
        return *this;
    }

    /**
     * Compares two msb_ts for equality.
     *
     * @param other The msb_t to be compared with.
     * @return True if the two msb_ts are equal, false otherwise.
     */
    constexpr bool
    operator==(msb_t const& other)
    {
        return this->get() == other.get();
    }

    /**
     * Compares two msb_ts for less-than.
     *
     * @param other The msb_t to be compared with.
     * @return True if the value of this msb_t is less than the value of the other msb_t, false otherwise.
     */
    constexpr bool
    operator<(msb_t const& other)
    {
        return this->get() < other.get();
    }

    /**
     * Compares two msb_ts for greater-than.
     *
     * @param other The msb_t to be compared with.
     * @return True if the value of this msb_t is greater than the value of the other msb_t, false otherwise.
     */
    constexpr bool
    operator>(msb_t const& other)
    {
        return this->get() > other.get();
    }

    /**
     * Compares two msb_ts for less-than-or-equal.
     *
     * @param other The msb_t to be compared with.
     * @return True if the value of this msb_t is less than or equal to the value of the other msb_t, false otherwise.
     */
    constexpr bool
    operator<=(msb_t const& other)
    {
        return this->get() <= other.get();
    }

    /**
     * Compares two msb_ts for greater-than-or-equal.
     *
     * @param other The msb_t to be compared with.
     * @return True if the value of this msb_t is greater than or equal to the value of the other msb_t, false
     * otherwise.
     */
    constexpr bool
    operator>=(msb_t const& other)
    {
        return this->get() >= other.get();
    }

private:
    /**
     * The actual value stored in the msb_t.
     */
    data_type value_{};
};

}    // namespace xitren::func
