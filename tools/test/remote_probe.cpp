#include <tm_session/remote_session.h>

#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const quint16 port = (argc > 1) ? static_cast<quint16>(QString(argv[1]).toUShort()) : 4300;

    opentm::tm_ui::remote_session s;
    int failures = 0;

    QObject::connect(&s, &opentm::tm_ui::session_api::error, [&](const QString& m) { out << "  error: " << m << Qt::endl; });

    if (!s.attach_tcp(QStringLiteral("127.0.0.1"), port)) {
        out << "FAIL: could not attach to 127.0.0.1:" << port << Qt::endl;
        return 1;
    }
    out << "attached to 127.0.0.1:" << port << Qt::endl;

    QTimer::singleShot(400, [&] {
        s.set_target([]{
            opentm::tm_ui::target_record r;
            r.name = QStringLiteral("probe");
            r.host = QStringLiteral("192.0.2.11");
            r.type = opentm::tm_core::target_type::cfw_dex;
            r.port = 1000;
            return r;
        }());
    });

    QTimer::singleShot(900, [&] {
        out << "  is_attached      : " << (s.is_attached() ? "yes" : "no") << Qt::endl;
        out << "  mirrored host    : " << s.target().host << Qt::endl;
        out << "  mirrored port    : " << s.target().port << Qt::endl;
        out << "  is_connected     : " << (s.is_connected() ? "yes" : "no") << Qt::endl;
        out << "  is_session_ready : " << (s.is_session_ready() ? "yes" : "no") << Qt::endl;

        if (s.target().host != QLatin1String("192.0.2.11")) {
            out << "FAIL: target not mirrored" << Qt::endl;
            ++failures;
        }
        out << "  requesting fs.list without a session..." << Qt::endl;
        s.list_directory(QStringLiteral("/"));
    });

    QTimer::singleShot(1500, [&] {
        out << (failures ? "PROBE FAILED" : "PROBE OK") << Qt::endl;
        app.exit(failures ? 1 : 0);
    });

    return app.exec();
}
