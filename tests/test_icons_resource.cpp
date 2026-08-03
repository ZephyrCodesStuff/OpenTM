#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QImage>
#include <QImageReader>

namespace {

const char* const kIcons[] = {
    "connect.bmp", "disconnect.bmp", "arrow_refresh.bmp",
    "control_play_blue.bmp", "control_stop_blue.bmp", "lightning.bmp",
    "application_go.bmp", "package.bmp", "add.bmp", "computer_edit.bmp",
    "computer_delete.bmp", "cog.bmp", "page_white_get.bmp",
    "page_white_put.bmp", "folder_explore.bmp", "table_gear.bmp",
    "text_align_left.bmp",
    "drive.bmp", "drive_disk.bmp", "drive_usb.bmp", "drive_ms.bmp",
    "drive_sd.bmp", "drive_cf.bmp", "drive_flash.bmp",
};

} // namespace

TEST_CASE("icon resources are present", "[icons]") {
    for (const char* name : kIcons) {
        const QString path = QStringLiteral(":/icons/") + QLatin1String(name);
        INFO(path.toStdString());
        QFile f(path);
        CHECK(f.exists());
        CHECK(f.size() > 0);
    }
}

TEST_CASE("icon resources actually decode", "[icons]") {
    QString formats;
    for (const QByteArray& f : QImageReader::supportedImageFormats()) {
        formats += QString::fromLatin1(f) + QLatin1Char(' ');
    }
    INFO("supported image formats: " << formats.toStdString());

    for (const char* name : kIcons) {
        const QString path = QStringLiteral(":/icons/") + QLatin1String(name);
        INFO(path.toStdString());
        QImage img;
        CHECK(img.load(path));
        CHECK_FALSE(img.isNull());
        CHECK(img.hasAlphaChannel());
    }
}
