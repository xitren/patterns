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

namespace xitren::func {

/// Returns true if the current CPU is little endian, false otherwise.
///
/// This function uses a compile-time constant expression to determine if the current CPU is little endian. It does so
/// by creating a 16-bit integer value with the least and most significant bytes swapped, and then checking if the least
/// significant byte is equal to 0x22. If the check passes, the current CPU is little endian, and the function returns
/// true. Otherwise, it is big endian, and the function returns false.
///
/// This function can be used to optimize code for different endianness architectures, by allowing the compiler to make
/// decisions at compile time based on the endianness of the CPU. This can lead to improved performance and reduced code
/// size.
///
/// @return true if the current CPU is little endian, false otherwise.
static constexpr bool
is_lsb()
{
    std::uint16_t               test{0x3322};
    std::array<std::uint8_t, 2> arr{};
    std::copy(&test, (&test) + 1, arr.begin());
    return arr[0] == 0x22;
}

template <class Type>
/**
 * A union that can be used to represent a value of a given type in memory as an array of bytes.
 *
 * The data is stored in two ways: as a set of fields, and as a set of bytes. This allows for efficient
 * serialization and deserialization of the data, as well as for interoperation with other data structures
 * that use different representations for the same data.
 *
 * The template parameter Type specifies the type of data that the union is intended to hold.
 */
union data {
    /** The actual data stored in the union. */
    Type fields;
    /** The data stored as an array of bytes. */
    std::array<std::uint8_t, sizeof(fields)> pure;

    /**
     * Deserialize a value of type Type from an input iterator.
     *
     * @param begin The input iterator pointing to the beginning of the data to be deserialized.
     * @return The deserialized value of type Type.
     */
    template <std::input_iterator InputIterator>
    static constexpr Type
    deserialize(InputIterator begin) noexcept
    {
        data<Type> tt{};
        std::copy(begin, begin + sizeof(fields), tt.pure.begin());
        return tt.fields;
    }

    /**
     * Serialize a value of type Type into an array of bytes.
     *
     * @param fields The value of type Type to be serialized.
     * @return The serialized data as an array of bytes.
     */
    static constexpr std::array<std::uint8_t, sizeof(fields)>
    serialize(Type const& fields) noexcept
    {
        data<Type> tt{fields};
        return tt.pure;
    }

    /**
     * Serialize a value of type Type into an input iterator.
     *
     * @param type The value of type Type to be serialized.
     * @param begin The input iterator pointing to the beginning of the data to be written.
     */
    template <std::input_iterator InputIterator>
    static constexpr void
    serialize(Type const& type, InputIterator begin) noexcept
    {
        data<Type> tt{type};
        std::copy(tt.pure.begin(), tt.pure.end(), begin);
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
    data<std::uint16_t> l{val};
    std::swap(l.pure[0], l.pure[1]);
    return l.fields;
}

/**
 * swaps the bytes of a 32-bit unsigned integer
 * @param val the value to be swapped
 * @return the swapped value
 */
static constexpr std::uint32_t
swap(std::uint32_t val) noexcept
{
    data<std::uint32_t> l{val};
    std::swap(l.pure[0], l.pure[3]);
    std::swap(l.pure[1], l.pure[2]);
    return l.fields;
}

/**
 * swaps the bytes of a 64-bit unsigned integer
 * @param val the value to be swapped
 * @return the swapped value
 */
static constexpr std::uint64_t
swap(std::uint64_t val) noexcept
{
    data<std::uint64_t> l{val};
    std::swap(l.pure[0], l.pure[7]);
    std::swap(l.pure[1], l.pure[6]);
    std::swap(l.pure[2], l.pure[5]);
    std::swap(l.pure[3], l.pure[4]);
    return l.fields;
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
