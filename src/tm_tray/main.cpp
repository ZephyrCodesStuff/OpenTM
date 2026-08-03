#include "tray_supervisor.h"

#include <tm_launch/launch.h>
#include <tm_ui/app_style.h>
#include <tm_launch/single_instance.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSystemTrayIcon>
#include <QTextStream>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("OpenTM"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenTM"));
    opentm::tm_ui::apply_saved_style();
    QApplication::setQuitOnLastWindowClosed(false);

    QCommandLineParser p;
    p.setApplicationDescription(QStringLiteral("OpenTM tray supervisor. Owns the session server so consoles stay ""connected while no window is open."));
    p.addHelpOption();
    const QCommandLineOption o_no_gui(QStringLiteral("no-gui"), QStringLiteral("Do not open the Target Manager window on start. Used ""when the window itself started this process."));
    const QCommandLineOption o_port({QStringLiteral("p"), QStringLiteral("port")}, QStringLiteral("Port the session server listens on (loopback)."), QStringLiteral("port"), QStringLiteral("4300"));
    const QCommandLineOption o_read_only(QStringLiteral("read-only"), QStringLiteral("Start the server without --allow-mutating. The GUI ""cannot load, install, reset or power a console."));
    p.addOption(o_no_gui);
    p.addOption(o_port);
    p.addOption(o_read_only);
    p.process(app);

    QTextStream out(stdout);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        out << "error: no system tray on this desktop; run opentm_server ""directly instead" << Qt::endl;
        return 1;
    }

    opentm::tm_tray::tray_supervisor sup;
    sup.set_port(static_cast<quint16>(p.value(o_port).toUShort()));
    sup.set_allow_mutating(!p.isSet(o_read_only));

    if (!sup.start(!p.isSet(o_no_gui))) {
        if (!p.isSet(o_no_gui)) {
            opentm::tm_launch::single_instance::request(QLatin1String(opentm::tm_launch::supervisor_socket), QJsonDocument(QJsonObject{{"method", "open_gui"}}).toJson(QJsonDocument::Compact));
        }
        out << "a supervisor is already running" << Qt::endl;
        return 0;
    }

    return app.exec();
}
