#include "single_instance.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QRegularExpression>

namespace opentm::tm_launch {

single_instance::single_instance(QObject* parent) : QObject(parent) {}
single_instance::~single_instance() = default;

QString single_instance::qualified(const QString& key) {
    QString user = qEnvironmentVariable("USER");
    if (user.isEmpty()) user = qEnvironmentVariable("USERNAME");
    if (user.isEmpty()) user = QStringLiteral("default");
    user.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")), QStringLiteral("_")); //let's hope this works on all three OSes
    return QStringLiteral("%1-%2").arg(key, user);
}

bool single_instance::acquire(const QString& key) {
    const auto name = qualified(key);
    {
        QLocalSocket probe;
        probe.connectToServer(name);
        if (probe.waitForConnected(500)) return false;
    }
    QLocalServer::removeServer(name);

    server_ = new QLocalServer(this);
    if (!server_->listen(name)) {
        delete server_;
        server_ = nullptr;
        return false;
    }
    connect(server_, &QLocalServer::newConnection, this, &single_instance::on_new_connection);
    return true;
}

void single_instance::on_new_connection() {
    while (auto* peer = server_->nextPendingConnection()) {
        connect(peer, &QLocalSocket::readyRead, this, [this, peer] {
            while (peer->canReadLine()) {
                const auto line = peer->readLine().trimmed();
                if (line.isEmpty()) continue;
                emit request_received(line, peer);
                peer->flush();
                peer->disconnectFromServer();
            }
        });
        connect(peer, &QLocalSocket::disconnected, peer, &QObject::deleteLater);
    }
}

QByteArray single_instance::request(const QString& key, const QByteArray& line, int timeout_ms)
{
    QLocalSocket s;
    s.connectToServer(qualified(key));
    if (!s.waitForConnected(timeout_ms)) return {};

    s.write(line.trimmed());
    s.write("\n");
    if (!s.waitForBytesWritten(timeout_ms)) return {};

    QByteArray reply;
    while (!reply.contains('\n')) {
        if (!s.waitForReadyRead(timeout_ms)) break;
        reply += s.readAll();
    }
    return reply.trimmed();
}

} // namespace opentm::tm_launch
