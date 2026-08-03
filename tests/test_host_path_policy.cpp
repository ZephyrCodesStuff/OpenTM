#include <catch2/catch_test_macros.hpp>

#include <tm_session/host_file_server.h>

#include <QDir>
#include <QString>
#include <QStringList>

using opentm::tm_ui::host_file_server;
TEST_CASE("a transfer authorises its path and its ancestors", "[host_paths]") {
    const auto chain = host_file_server::transfer_path_chain("C:/Users/dev/Desktop/downloads/boot_plugins.txt");

    CHECK(chain.contains("C:/Users/dev/Desktop/downloads/boot_plugins.txt"));
    // the target stats its way down before opening, so these must resolve too
    CHECK(chain.contains("C:/Users/dev/Desktop/downloads"));
    CHECK(chain.contains("C:/Users/dev/Desktop"));
    CHECK(chain.contains("C:/Users/dev"));
    CHECK(chain.contains("C:/Users"));

    SECTION("the walk reaches the drive root, which the target does ask for") {
        CHECK(chain.contains("C:/"));
    }

    SECTION("a sibling of the authorised file is not authorised") {
        CHECK_FALSE(chain.contains("C:/Users/dev/Desktop/downloads/secrets.txt"));
        CHECK_FALSE(chain.contains("C:/Users/other"));
    }

    SECTION("backslashes reach the same set") {
        const auto win = host_file_server::transfer_path_chain("C:\\Users\\dev\\Desktop\\downloads\\boot_plugins.txt");
        CHECK(win == chain);
    }
}

TEST_CASE("a bare drive letter is absolute and means its root", "[host_paths]") {
    CHECK(host_file_server::looks_absolute("C:"));
    CHECK(host_file_server::looks_absolute("C:/"));
    CHECK(host_file_server::looks_absolute("C:/Users"));
    CHECK(host_file_server::looks_absolute("/dev_hdd0"));

    CHECK(host_file_server::normalise_host_path("C:") == "C:/");
    CHECK(host_file_server::normalise_host_path("C:/") == "C:/");
    CHECK(host_file_server::normalise_host_path("C:\\") == "C:/");
}

TEST_CASE("relative paths stay relative", "[host_paths]") {
    CHECK_FALSE(host_file_server::looks_absolute("EBOOT.BIN"));
    CHECK_FALSE(host_file_server::looks_absolute("USRDIR/EBOOT.BIN"));
    CHECK_FALSE(host_file_server::looks_absolute("PS3SETTINGS.SFT"));
    CHECK_FALSE(host_file_server::looks_absolute(""));
    CHECK_FALSE(host_file_server::looks_absolute("1:foo"));
}

TEST_CASE("normalisation collapses traversal before any check", "[host_paths]") {
    CHECK(host_file_server::normalise_host_path("C:/a/b/../c") == "C:/a/c");
    CHECK(host_file_server::normalise_host_path("C:/a/./b") == "C:/a/b");

    const auto chain = host_file_server::transfer_path_chain("C:/a/b/../c/f.txt");
    CHECK(chain.contains("C:/a/c/f.txt"));
    CHECK_FALSE(chain.contains("C:/a/b"));
}
