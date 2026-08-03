#include <tm_core/target_type.h>

#include <catch2/catch_test_macros.hpp>

using namespace opentm::tm_core;

TEST_CASE("target_type string round-trip", "[target_type]") {
    CHECK(target_type_string(target_type::decr_tcp)  == "PS3_DEH_TCP");
    CHECK(target_type_string(target_type::cfw_dex)   == "PS3_DBG_DEX");
    CHECK(target_type_string(target_type::core_dump) == "PS3_CORE_DUMP");

    CHECK(target_type_from_string("PS3_DEH_TCP")   == target_type::decr_tcp);
    CHECK(target_type_from_string("PS3_DBG_DEX")   == target_type::cfw_dex);
    CHECK(target_type_from_string("PS3_CORE_DUMP") == target_type::core_dump);
    CHECK_FALSE(target_type_from_string("garbage").has_value());
}

TEST_CASE("default_deci3_port matches observed ports", "[target_type]") {
    CHECK(default_deci3_port(target_type::decr_tcp) == 8530);
    CHECK(default_deci3_port(target_type::cfw_dex)  == 1000);
    CHECK(default_deci3_port(target_type::core_dump) == 0);
}
