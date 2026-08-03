#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>

#include <functional>

class QIODevice;

namespace opentm::tm_ui {

class rpc_client : public QObject {
    Q_OBJECT
public:
    explicit rpc_client(QObject* parent = nullptr);
    ~rpc_client() override;

    bool connect_tcp(const QString& host, quint16 port, int timeout_ms = 3000);
    bool connect_local(const QString& name, int timeout_ms = 3000);
    void disconnect_from_server();
    bool is_connected() const;

    using reply_handler = std::function<void(bool ok, const QJsonObject& result, const QString& error)>;
    void call(const QString& method, const QJsonObject& params = {}, reply_handler on_reply = {});

signals:
    void event_received(QString target, QString name, QJsonObject params);
    void connected();
    void disconnected();
    void protocol_error(QString message);

private:
    void attach(QIODevice* dev);
    void on_ready_read();
    void handle_message(const QJsonObject& msg);

    QIODevice*  dev_ = nullptr;
    QByteArray  buf_;
    int         next_id_ = 0;
    QHash<int, reply_handler> pending_;
};

} // namespace opentm::tm_ui
