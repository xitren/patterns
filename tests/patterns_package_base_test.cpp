#include <xitren/crc16.hpp>
#include <xitren/func/packet.hpp>

#include <gtest/gtest.h>

using namespace xitren::func;

class crc16ansi {
public:
    using value_type = lsb_t<std::uint16_t>;

    /**
     * @brief Calculates the CRC-16 ANSI checksum
     *
     * @tparam InputIterator An input iterator that points to a sequence of bytes
     * @param begin An iterator to the first element in the sequence
     * @param end An iterator to one past the last element in the sequence
     * @return The calculated CRC-16 ANSI checksum
     */
    template <xitren::crc::crc_iterator InputIterator>
    static constexpr value_type
    calculate(InputIterator begin, InputIterator end) noexcept
    {
        static_assert(sizeof(*begin) == sizeof(std::uint8_t));
        return xitren::crc::crc16::calculate(begin, end);
    }

    /**
     * @brief Calculates the CRC-16 ANSI checksum
     *
     * @tparam Size The size of the data array
     * @param data The data array
     * @return The calculated CRC-16 ANSI checksum
     */
    template <std::size_t Size>
    static constexpr value_type
    calculate(std::array<std::uint8_t, Size> const& data) noexcept
    {
        return xitren::crc::crc16::calculate(data);
    }
};

TEST(modbus_packet_test, crc16ansi)
{
    std::array<std::uint8_t, 5> data = {1, 2, 3, 4, 5};
    crc16ansi::value_type       crc(crc16ansi::calculate(data.begin(), data.end()));
    EXPECT_EQ(crc.get(), 47914);
}

struct header_ext {
    uint8_t magic_header[2];
};

struct noise_frame {
    static inline header_ext header = {'N', 'O'};
    uint16_t                 data;
};

struct adc_frame {
    static inline header_ext header = {'L', 'N'};
    uint16_t                 data[11];
};

TEST(package_base_test, packets_eq)
{
    noise_frame adf = {};

    for (auto i{0}; i < 1000; i++) {
        adf.data = static_cast<uint16_t>(std::rand());
        packet<header_ext, noise_frame, crc16ansi> pack(noise_frame::header, adf);
        packet<header_ext, noise_frame, crc16ansi> pack_recv(pack.to_array());
        ASSERT_EQ(noise_frame::header.magic_header[0], pack_recv.header().magic_header[0]);
        ASSERT_EQ(noise_frame::header.magic_header[1], pack_recv.header().magic_header[1]);
    }
}