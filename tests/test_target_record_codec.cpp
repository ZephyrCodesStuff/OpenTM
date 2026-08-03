#include <catch2/catch_test_macros.hpp>

#include <tm_session/target_record_codec.h>

#include <tm_core/wol.h>

using opentm::tm_ui::target_record;
using opentm::tm_ui::target_record_to_json;
using opentm::tm_ui::target_record_from_json;

TEST_CASE("target_record survives a round trip", "[target_record_codec]") {
    target_record in;
    in.id   = QStringLiteral("6f1c7b2e-0000-4000-8000-abcdefabcdef");
    in.name = QStringLiteral("CECHB002");
    in.host = QStringLiteral("192.0.2.37");
    in.port = 1000;
    in.type = opentm::tm_core::target_type::cfw_dex;
    in.mac  = QStringLiteral("00:11:22:33:44:55");
    in.file_server_dir = QStringLiteral("D:/builds/cellmark");
    in.home_dir        = QStringLiteral("D:/home");
    in.force_case_sensitive = true;
    in.env_var_expansion    = true;
    in.console_cache_kb     = 4096;
    in.file_trace_log_size  = 12345;
    in.image_capture_auto   = true;
    in.image_capture_dir    = QStringLiteral("D:/caps");
    in.timeouts.reconnect_ms = 4321;
    in.timeouts.load_ms      = 99000;
    in.load.disable_spu_debug = true;
    in.load.stack_size        = 0x40;
    in.load.cmdline           = QStringLiteral("-loglevel 3");

    const auto out = target_record_from_json(target_record_to_json(in));

    CHECK(out.id   == in.id);
    CHECK(out.name == in.name);
    CHECK(out.host == in.host);
    CHECK(out.port == in.port);
    CHECK(out.type == in.type);
    CHECK(out.mac  == in.mac);
    CHECK(out.file_server_dir == in.file_server_dir);
    CHECK(out.home_dir        == in.home_dir);
    CHECK(out.force_case_sensitive == in.force_case_sensitive);
    CHECK(out.env_var_expansion    == in.env_var_expansion);
    CHECK(out.console_cache_kb     == in.console_cache_kb);
    CHECK(out.file_trace_log_size  == in.file_trace_log_size);
    CHECK(out.image_capture_auto   == in.image_capture_auto);
    CHECK(out.image_capture_dir    == in.image_capture_dir);
    CHECK(out.timeouts.reconnect_ms == in.timeouts.reconnect_ms);
    CHECK(out.timeouts.load_ms      == in.timeouts.load_ms);
    CHECK(out.load.disable_spu_debug == in.load.disable_spu_debug);
    CHECK(out.load.stack_size        == in.load.stack_size);
    CHECK(out.load.cmdline           == in.load.cmdline);
}

TEST_CASE("64-bit reset values survive the JSON hop", "[target_record_codec]") {
    target_record in;
    in.reset_mode         = target_record::reset_advanced;
    in.reset_boot_value   = 0x0123456789ABCDEFull;
    in.reset_boot_mask    = 0xFEDCBA9876543210ull;
    in.reset_system_value = 0xFFFFFFFFFFFFFFFFull;
    in.reset_system_mask  = 0x8000000000000001ull;

    const auto out = target_record_from_json(target_record_to_json(in));

    CHECK(out.reset_mode         == target_record::reset_advanced);
    CHECK(out.reset_boot_value   == in.reset_boot_value);
    CHECK(out.reset_boot_mask    == in.reset_boot_mask);
    CHECK(out.reset_system_value == in.reset_system_value);
    CHECK(out.reset_system_mask  == in.reset_system_mask);
}

TEST_CASE("absent fields keep their defaults", "[target_record_codec]") {
    const target_record defaults;
    const auto out = target_record_from_json(QJsonObject{});

    CHECK(out.port                 == defaults.port);
    CHECK(out.type                 == defaults.type);
    CHECK(out.console_cache_kb     == defaults.console_cache_kb);
    CHECK(out.events_to_log        == defaults.events_to_log);
    CHECK(out.reset_boot_value     == defaults.reset_boot_value);
    CHECK(out.timeouts.connect_ms  == defaults.timeouts.connect_ms);
    CHECK(out.display_reset_settings == defaults.display_reset_settings);
}
