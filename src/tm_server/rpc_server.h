#pragma once

#include <tm_session/target_session.h>

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

class QIODevice;
class QLocalServer;
class QTcpServer;

namespace opentm::tm_server {

class rpc_server : public QObject {
    Q_OBJECT
public:
    explicit rpc_server(QObject* parent = nullptr);
    ~rpc_server() override;
    void set_allow_mutating(bool on) { allow_mutating_ = on; }
    void set_exit_with_owner(bool on) { exit_with_owner_ = on; }
    bool listen_tcp(quint16 port);
    bool listen_local(const QString& name);
    void announce_shutdown(const QString& reason);

signals:
    void log_message(QString line);

private:
    struct managed {
        opentm::tm_ui::target_session* session = nullptr;
        QString     handle;
        QString     name;
        QStringList tty;
    };

    struct method {
        std::function<QJsonObject(managed*, const QJsonObject&, QString&)> fn;
        bool        mutating      = false;
        bool        needs_target  = false;
        bool        needs_session = false;
        const char* help          = "";
    };

    void register_methods();
    void wire_session_events(managed* m);
    static QJsonObject status_json(const managed* m);

    managed* open_target(const QJsonObject& params, QString& err);
    managed* resolve_target(const QJsonObject& params, QString& err);
    void close_target(const QString& handle);

    void on_new_tcp_client();
    void on_new_local_client();
    void attach(QIODevice* dev);
    void on_ready_read(QIODevice* dev);
    void handle_line(QIODevice* dev, const QByteArray& line);

    void send(QIODevice* dev, const QJsonObject& obj);
    void broadcast_event(const QString& target, const QString& name, const QJsonObject& params);

    QTcpServer*        tcp_   = nullptr;
    QLocalServer*      local_ = nullptr;
    QList<QIODevice*>  clients_;
    QHash<QIODevice*, QByteArray> buffers_;
    QHash<QString, method>        methods_;
    QHash<QString, managed*>      sessions_;
    QIODevice*                    owner_ = nullptr;
    QIODevice*                    current_client_ = nullptr;

    bool allow_mutating_   = false;
    bool exit_with_owner_  = false;
    bool announced_        = false;
    static constexpr int kMaxTtyLines = 5000;
};

} // namespace opentm::tm_server
