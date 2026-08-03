#include "rpc_server.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("opentm_server"));

    QTextStream out(stdout);

    QCommandLineParser p;
    p.setApplicationDescription(QStringLiteral("Session server for PS3 devkits and DEX consoles. ""Clients speak line-delimited JSON."));
    p.addHelpOption();
    const QCommandLineOption o_port({"p", "port"}, "TCP port on 127.0.0.1 (default 4300).", "port", "4300");
    const QCommandLineOption o_local("local", "Also listen on a local socket (named pipe / unix socket).", "name");
    const QCommandLineOption o_no_tcp("no-tcp", "Skip the TCP listener.");
    const QCommandLineOption o_allow("allow-mutating", "Permit verbs that change target state (load, install, reset, power).");
    const QCommandLineOption o_quiet("quiet", "Don't echo session logs to stdout.");
    const QCommandLineOption o_own("exit-with-owner", "Quit when the client that calls server.own disconnects. Used by the tray supervisor so killing it cannot leave this behind.");
    for (const auto& o : {o_port, o_local, o_no_tcp, o_allow, o_quiet, o_own}) p.addOption(o);
    p.process(app);

    opentm::tm_server::rpc_server server;
    server.set_allow_mutating(p.isSet(o_allow));
    server.set_exit_with_owner(p.isSet(o_own));

    if (!p.isSet(o_quiet)) {
        QObject::connect(&server, &opentm::tm_server::rpc_server::log_message, [&out](const QString& l) { out << l << Qt::endl; });
    }

    bool listening = false;
    if (!p.isSet(o_no_tcp)) {
        listening |= server.listen_tcp(static_cast<quint16>(p.value(o_port).toUShort()));
    }
    if (p.isSet(o_local)) {
        listening |= server.listen_local(p.value(o_local));
    }
    if (!listening) {
        out << "error: no listener could be started" << Qt::endl;
        return 1;
    }

    if (!p.isSet(o_allow)) {
        out << "note: read-only. Pass --allow-mutating to permit load / " "install / reset / power." << Qt::endl;
    }
    return app.exec();
}
