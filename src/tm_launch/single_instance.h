#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class QLocalServer;
class QLocalSocket;

namespace opentm::tm_launch {

class single_instance : public QObject {
    Q_OBJECT
public:
    explicit single_instance(QObject* parent = nullptr);
    ~single_instance() override;

    bool acquire(const QString& key);

    static QByteArray request(const QString& key, const QByteArray& line, int timeout_ms = 3000);
    static QString qualified(const QString& key);

signals:
    void request_received(QByteArray line, QLocalSocket* peer);

private:
    void on_new_connection();

    QLocalServer* server_ = nullptr;
};

} // namespace opentm::tm_launch
