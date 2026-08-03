#include "app_prefs.h"
#include "app_style.h"
#include "main_window.h"

#include <tm_launch/launch.h>
#include <tm_launch/single_instance.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QLocalSocket>
#include <QTextStream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("OpenTM");
    QCoreApplication::setApplicationName("OpenTM");
    opentm::tm_ui::apply_saved_style();

    QTextStream out(stdout);

    QCommandLineParser p;
    p.setApplicationDescription(QStringLiteral("Target manager for PS3 devkits and DEX consoles.\n\n""By default this starts (or joins) a tray supervisor and session ""server, so consoles stay connected when the window closes and can ""be shared with a debugger."));
    p.addHelpOption();
    const QCommandLineOption o_standalone(QStringLiteral("standalone"), QStringLiteral("Own the console sessions in this process. No tray, no " "server; sessions end when the window closes."));
    const QCommandLineOption o_server(QStringLiteral("server"), QStringLiteral("Use the session server at this address instead of " "starting one."), QStringLiteral("host:port"), QStringLiteral("127.0.0.1:4300"));
    const QCommandLineOption o_server_local(QStringLiteral("server-local"), QStringLiteral("Same, over a local socket / named pipe."), QStringLiteral("name"));
    p.addOption(o_standalone);
    p.addOption(o_server);
    p.addOption(o_server_local);
    p.process(app);

    opentm::tm_launch::single_instance instance;
    if (!instance.acquire(QLatin1String(opentm::tm_launch::gui_socket))) {
        opentm::tm_launch::single_instance::request(QLatin1String(opentm::tm_launch::gui_socket), "raise");
        return 0;
    }

    opentm::tm_ui::session_backend backend;
    if (p.isSet(o_standalone)) {
    } else if (p.isSet(o_server_local)) {
        backend.k = opentm::tm_ui::session_backend::kind::remote_local;
        backend.local_name = p.value(o_server_local);
    } else if (p.isSet(o_server)) {
        backend.k = opentm::tm_ui::session_backend::kind::remote_tcp;
        const auto value = p.value(o_server);
        const int colon = value.lastIndexOf(QLatin1Char(':'));
        if (colon > 0) {
            backend.host = value.left(colon);
            backend.port = static_cast<quint16>(value.mid(colon + 1).toUShort());
        } else {
            backend.host = value;
        }
    } else if (!opentm::tm_ui::use_tray_supervisor()) {
        out << "note: background sessions are off in Preferences; running standalone" << Qt::endl;
    } else {
        QString err;
        quint16 port = 0;
        if (opentm::tm_launch::ensure_supervisor(&err)) {
            port = opentm::tm_launch::ensure_server(&err);
        }
        if (port != 0) {
            backend.k    = opentm::tm_ui::session_backend::kind::remote_tcp;
            backend.host = QStringLiteral("127.0.0.1");
            backend.port = port;
        } else {
            backend.fallback_reason = err;
            out << "note: " << err << Qt::endl;
        }
    }

    opentm::tm_ui::main_window w(backend);

    QObject::connect(&instance, &opentm::tm_launch::single_instance::request_received, &w, [&w](const QByteArray& line, QLocalSocket* peer) {
        if (line == "raise") {
            w.setWindowState((w.windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            w.show();
            w.raise();
            w.activateWindow();
        } else if (line == "quit") {
            peer->write("ok\n");
            peer->flush();
            w.close();
            return;
        }
        peer->write("ok\n");
    });

    w.show();
    return app.exec();
}