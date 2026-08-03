#include <tm_core/deci3_codec.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <vector>

using namespace opentm::tm_core;

namespace {

template <typename... Ts>
std::vector<std::byte> bytes_of(Ts... vs) {
    return std::vector<std::byte>{static_cast<std::byte>(static_cast<std::uint8_t>(vs))...};
}

} // namespace

TEST_CASE("deci3 envelope round-trips a netmp get-version request", "[deci3][codec]") {
    const std::vector<std::byte> wire = bytes_of(
        0x30, 0x10,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x14,
        0x48, 0x4d,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x10,
        0x10, 0x00, 0x00, 0x00
    );

    auto r = decode_frame(wire);
    REQUIRE(r.frame.has_value());
    REQUIRE_FALSE(r.error.has_value());

    const auto& f = *r.frame;
    CHECK(f.session_a == 0u);
    CHECK(f.direction == deci3_direction::host_to_machine);
    CHECK(family_of(f.direction) == deci3_family::netmp);
    CHECK(f.session_b == 0u);
    CHECK(f.category == 0x0010);
    REQUIRE(f.payload.size() == 4);
    CHECK(std::to_integer<std::uint8_t>(f.payload[0]) == 0x10);
    CHECK(r.consumed == wire.size());

    std::vector<std::byte> reencoded;
    const auto wrote = encode_frame(f, reencoded);
    CHECK(wrote == wire.size());
    CHECK(reencoded == wire);
}

TEST_CASE("deci3 envelope round-trips a netmp get-version reply", "[deci3][codec]") {
    const std::vector<std::byte> wire = bytes_of(
        0x30, 0x10,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x18,
        0x4d, 0x48,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x10,
        0x11, 0x00, 0x32, 0x2e, 0x31, 0x2e, 0x32, 0x00
    );

    auto r = decode_frame(wire);
    REQUIRE(r.frame.has_value());
    const auto& f = *r.frame;
    CHECK(f.direction == deci3_direction::machine_to_host);
    CHECK(family_of(f.direction) == deci3_family::netmp);
    CHECK(f.category == 0x0010);
    REQUIRE(f.payload.size() == 8);
    CHECK(std::to_integer<std::uint8_t>(f.payload[2]) == '2');
    CHECK(std::to_integer<std::uint8_t>(f.payload[3]) == '.');
    CHECK(std::to_integer<std::uint8_t>(f.payload[4]) == '1');
    CHECK(std::to_integer<std::uint8_t>(f.payload[5]) == '.');
    CHECK(std::to_integer<std::uint8_t>(f.payload[6]) == '2');
    CHECK(std::to_integer<std::uint8_t>(f.payload[7]) == 0x00);
}

TEST_CASE("deci3 envelope recognises every direction family", "[deci3][codec]") {
    CHECK(family_of(deci3_direction::host_to_machine)    == deci3_family::netmp);
    CHECK(family_of(deci3_direction::machine_to_host)    == deci3_family::netmp);
    CHECK(family_of(deci3_direction::host_to_target)     == deci3_family::dfmp);
    CHECK(family_of(deci3_direction::target_to_host)     == deci3_family::dfmp);
    CHECK(family_of(deci3_direction::manager_to_target)  == deci3_family::netmp_cfw);
    CHECK(family_of(deci3_direction::target_to_manager)  == deci3_family::netmp_cfw);

    CHECK(is_host_originated(deci3_direction::host_to_machine));
    CHECK(is_host_originated(deci3_direction::host_to_target));
    CHECK(is_host_originated(deci3_direction::manager_to_target));
    CHECK_FALSE(is_host_originated(deci3_direction::machine_to_host));
    CHECK_FALSE(is_host_originated(deci3_direction::target_to_host));
    CHECK_FALSE(is_host_originated(deci3_direction::target_to_manager));
}

TEST_CASE("deci3 decoder rejects bad magic and short buffers", "[deci3][codec]") {
    SECTION("short buffer") {
        std::vector<std::byte> b(8, std::byte{0});
        auto r = decode_frame(b);
        CHECK_FALSE(r.frame.has_value());
        REQUIRE(r.error.has_value());
        CHECK(*r.error == decode_error::short_buffer);
    }
    SECTION("bad magic") {
        auto b = bytes_of(
            0xde, 0xad,
            0,0,0,0,
            0,16,
            'H','M',
            0,0,0,0,
            0,0x10
        );
        auto r = decode_frame(b);
        CHECK_FALSE(r.frame.has_value());
        REQUIRE(r.error.has_value());
        CHECK(*r.error == decode_error::bad_magic);
    }
    SECTION("length larger than buffer") {
        auto b = bytes_of(
            0x30, 0x10,
            0,0,0,0,
            0x00, 0x40, // claim 64 bytes total
            'H','M',
            0,0,0,0,
            0,0x10,
            0,0,0,0  // only 20 bytes actually present
        );
        auto r = decode_frame(b);
        CHECK_FALSE(r.frame.has_value());
        REQUIRE(r.error.has_value());
        CHECK(*r.error == decode_error::bad_length);
    }
}

TEST_CASE("decode_all walks back-to-back frames", "[deci3][codec]") {
    const std::vector<std::byte> wire = bytes_of(
        0x30,0x10, 0,0,0,0, 0,0x14, 'H','M', 0,0,0,0, 0,0x10, 0x10,0,0,0,
        0x30,0x10, 0,0,0,0, 0,0x14, 'H','M', 0,0,0,0, 0,0x10, 0x10,0,0,0
    );
    std::optional<decode_error> err;
    std::size_t off = 0;
    auto frames = decode_all(wire, &err, &off);
    CHECK_FALSE(err.has_value());
    REQUIRE(frames.size() == 2);
    CHECK(off == wire.size());
}
