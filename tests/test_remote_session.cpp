#include <catch2/catch_test_macros.hpp>

#include <tm_session/remote_session.h>
#include <tm_session/target_record.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

using namespace opentm;

namespace {

class stub_server {
public:
    bool listen() {
        QObject::connect(&server_, &QTcpServer::newConnection, [this] {
            client_ = server_.nextPendingConnection();
            QObject::connect(client_, &QTcpSocket::readyRead, [this] { drain(); });
        });
        for (quint16 port = 43700; port < 43760; ++port) {
            if (server_.listen(QHostAddress::LocalHost, port)) { port_ = port; return true; }
        }
        return false;
    }

    quint16 port() const { return port_; }
    const QStringList& methods() const { return methods_; }
    void fail_open(bool on) { fail_open_ = on; }

private:
    void drain() {
        buffer_ += client_->readAll();
        int nl = 0;
        while ((nl = buffer_.indexOf('\n')) >= 0) {
            const auto line = buffer_.left(nl);
            buffer_.remove(0, nl + 1);
            handle(QJsonDocument::fromJson(line).object());
        }
    }

    void reply(int id, bool ok, const QJsonObject& result, const QString& error) {
        if (!client_) return;
        QJsonObject o{{"id", id}, {"ok", ok}};
        if (ok) o.insert("result", result); else o.insert("error", error);
        client_->write(QJsonDocument(o).toJson(QJsonDocument::Compact) + "\n");
    }

    void handle(const QJsonObject& req) {
        const auto method = req.value("method").toString();
        const int  id     = req.value("id").toInt();
        methods_ << method;

        if (method == QStringLiteral("target.open")) {
            QTimer::singleShot(120, [this, id] {
                if (fail_open_) {
                    reply(id, false, {}, QStringLiteral("target 'x' is already open on 10.0.0.235:8530"));
                } else {
                    reply(id, true, QJsonObject{{"target", QStringLiteral("handle-1")}}, {});
                }
            });
            return;
        }
        if (method == QStringLiteral("server.version")) {
            reply(id, true, QJsonObject{{"build", QStringLiteral("test")}, {"debugger", true}}, {});
            return;
        }
        reply(id, true, {}, {});
    }

    QTcpServer  server_;
    QTcpSocket* client_ = nullptr;
    QByteArray  buffer_;
    QStringList methods_;
    quint16     port_ = 0;
    bool        fail_open_ = false;
};

void ensure_app() {
    if (QCoreApplication::instance()) return;
    static int   argc = 1;
    static char  name[] = "tm_core_tests";
    static char* argv[] = {name, nullptr};
    new QCoreApplication(argc, argv);
}

void pump(int ms) {
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
}

tm_ui::target_record dex_target(quint16 port) {
    tm_ui::target_record r;
    r.id   = QStringLiteral("585a50c5-12d7-4544-910b-2c36042317ad");
    r.name = QStringLiteral("PS3");
    r.host = QStringLiteral("10.0.0.235");
    r.port = port;
    r.type = tm_core::target_type::cfw_dex;
    return r;
}

} // namespace

TEST_CASE("connect asked for while target.open is in flight still connects", "[remote_session]") {
    ensure_app();
    stub_server stub;
    REQUIRE(stub.listen());

    tm_ui::remote_session session;
    REQUIRE(session.attach_tcp(QStringLiteral("127.0.0.1"), stub.port()));
    pump(50);

    QStringList errors;
    QObject::connect(&session, &tm_ui::session_api::error, [&errors](const QString& e) { errors << e; });

    session.set_target(dex_target(1000));
    session.connect_to_target();

    CHECK(session.peer_summary() == QStringLiteral("10.0.0.235:1000"));

    CHECK(session.connection_state() != tm_core::tcp_connection::state::disconnected);
    CHECK_FALSE(stub.methods().contains(QStringLiteral("target.connect")));

    pump(400);

    INFO("methods seen: " << stub.methods().join(QLatin1Char(',')).toStdString());
    INFO("errors: " << errors.join(QLatin1Char('|')).toStdString());
    for (const auto& e : errors) {
        CHECK_FALSE(e.contains(QStringLiteral("nothing to connect")));
    }
    CHECK(stub.methods().contains(QStringLiteral("target.connect")));
}

TEST_CASE("a refused target.open does not connect anyway", "[remote_session]") {
    ensure_app();
    stub_server stub;
    REQUIRE(stub.listen());
    stub.fail_open(true);

    tm_ui::remote_session session;
    REQUIRE(session.attach_tcp(QStringLiteral("127.0.0.1"), stub.port()));
    pump(50);

    session.set_target(dex_target(1000));
    session.connect_to_target();
    pump(400);

    INFO("methods seen: " << stub.methods().join(QLatin1Char(',')).toStdString());
    CHECK_FALSE(stub.methods().contains(QStringLiteral("target.connect")));
    CHECK(session.connection_state() == tm_core::tcp_connection::state::disconnected);
}