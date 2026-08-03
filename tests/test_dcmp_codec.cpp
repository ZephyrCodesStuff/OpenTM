#include <catch2/catch_test_macros.hpp>

#include <tm_core/dcmp_codec.h>

#include <string>

using namespace opentm::tm_core::dcmp;

TEST_CASE("DCMP status codes", "[dcmp]") {
    CHECK(std::string(status_name(status_code::connected))       == "CONNECTED");
    CHECK(std::string(status_name(status_code::proto))           == "PROTO");
    CHECK(std::string(status_name(status_code::delete_proto))    == "DELETE_PROTO");
    CHECK(std::string(status_name(status_code::space))           == "SPACE");
    CHECK(std::string(status_name(status_code::system_boot))     == "SYSTEM_BOOT");
    CHECK(std::string(status_name(status_code::system_shutdown)) == "SYSTEM_SHUTDOWN");
    CHECK(std::string(status_name(status_code::system_suspend))  == "SYSTEM_SUSPEND");
    CHECK(std::string(status_name(status_code::system_resume))   == "SYSTEM_RESUME");
    CHECK(std::string(status_name(status_code::lpar_boot))       == "LPAR_BOOT");
    CHECK(std::string(status_name(status_code::lpar_resume))     == "LPAR_RESUME");
    CHECK(std::string(status_name(0xFF))                         == "UNKNOWN");
}

TEST_CASE("DCMP error codes", "[dcmp]") {
    CHECK(std::string(error_name(error_code::invalhead))        == "INVALHEAD");
    CHECK(std::string(error_name(error_code::system_off))       == "SYSTEM_OFF");
    CHECK(std::string(error_name(error_code::system_suspended)) == "SYSTEM_SUSPENDED");
    CHECK(std::string(error_name(error_code::lpar_none))        == "LPAR_NONE");
    CHECK(std::string(error_name(error_code::lpar_suspended))   == "LPAR_SUSPENDED");
    CHECK(std::string(error_name(error_code::noconnect))        == "NOCONNECT");
    CHECK(std::string(error_name(error_code::noproto))          == "NOPROTO");
    CHECK(std::string(error_name(error_code::priority))         == "PRIORITY");
    CHECK(std::string(error_name(error_code::nospace))          == "NOSPACE");
    CHECK(std::string(error_name(0xFF))                         == "UNKNOWN");
}

TEST_CASE("DCMP status drives the reconnect decision", "[dcmp]") {
    for (auto down : {status_code::system_shutdown, status_code::system_suspend, status_code::lpar_shutdown,   status_code::lpar_suspend}) {
        INFO(status_name(down));
        CHECK(status_means_going_down(down));
        CHECK_FALSE(status_means_coming_up(down));
    }
    for (auto up : {status_code::system_boot, status_code::system_resume, status_code::lpar_boot,   status_code::lpar_resume}) {
        INFO(status_name(up));
        CHECK(status_means_coming_up(up));
        CHECK_FALSE(status_means_going_down(up));
    }
    for (auto other : {status_code::connected, status_code::proto, status_code::delete_proto, status_code::space}) {
        INFO(status_name(other));
        CHECK_FALSE(status_means_going_down(other));
        CHECK_FALSE(status_means_coming_up(other));
    }
}
