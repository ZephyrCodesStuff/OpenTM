#include "target_discovery.h"

#include <tm_core/deci3_codec.h>

#include <QAbstractSocket>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>

#include <algorithm>

namespace opentm::tm_ui {

namespace {

constexpr int           scan_host_cap = 4096;
constexpr std::uint16_t port_decr     = 8530;
constexpr std::uint16_t port_b00      = 1000;

opentm::tm_core::target_type kind_for_port(std::uint16_t port) noexcept {
    if (port == port_decr) return opentm::tm_core::target_type::decr_tcp;
    if (port == port_b00)  return opentm::tm_core::target_type::cfw_dex;
    return opentm::tm_core::target_type::unknown;
}

} // namespace

target_discovery::target_discovery(QObject* parent) : QObject(parent) {}

target_discovery::~target_discovery() {
    cancel();
}

void target_discovery::scan(QList<QHostAddress> hosts) {
    cancel();
    if (hosts.size() > scan_host_cap) hosts = hosts.mid(0, scan_host_cap);

    // 2 probes per host (port_decr + port_b00).
    const int total = hosts.size() * 2;
    emit scan_started(total);
    if (hosts.isEmpty()) {
        emit scan_finished();
        return;
    }

    probes_.reserve(total);
    auto start_one = [this](const QHostAddress& host, std::uint16_t port) {
        auto* p = new probe;
        p->host  = host;
        p->port  = port;
        p->kind  = kind_for_port(port);
        p->sock  = new QTcpSocket(this);
        p->timer = new QTimer(this);
        p->timer->setSingleShot(true);
        probes_.append(p);

        connect(p->sock, &QTcpSocket::connected, this, [this, p]() { on_connected(p); });
        connect(p->sock, &QTcpSocket::readyRead, this, [this, p]() { on_probe_readable(p); });
        connect(p->sock, &QTcpSocket::errorOccurred, this, [this, p]() { finish_probe(p, false); });
        connect(p->timer, &QTimer::timeout, this, [this, p]() { on_timeout(p); });

        p->timer->start(connect_timeout_ms_);
        p->sock->connectToHost(host, port);
    };

    for (const auto& host : hosts) {
        start_one(host, port_decr);
        start_one(host, port_b00);
    }
}

void target_discovery::cancel() {
    for (auto* p : probes_) {
        p->timer->stop();
        p->timer->deleteLater();
        p->sock->abort();
        p->sock->deleteLater();
        delete p;
    }
    probes_.clear();
}

void target_discovery::on_connected(probe* p) {
    using namespace opentm::tm_core;
    deci3_frame f;
    f.direction = (p->kind == target_type::cfw_dex) ? deci3_direction::manager_to_target : deci3_direction::host_to_machine;
    f.category  = 0x0010;
    for (auto b : {0x10, 0x00, 0x00, 0x00}) {
        f.payload.push_back(std::byte{static_cast<std::uint8_t>(b)});
    }
    std::vector<std::byte> wire;
    encode_frame(f, wire);
    p->sock->write(reinterpret_cast<const char*>(wire.data()), static_cast<qint64>(wire.size()));
    // give the greeting its own window rather than whatever is left of the connect budget
    if (p->timer) p->timer->start(connect_timeout_ms_);
}

void target_discovery::on_probe_readable(probe* p) {
    const auto data = p->sock->readAll();
    if (data.size() < 2) return;
    p->greeted = static_cast<std::uint8_t>(data[0]) == 0x30 && static_cast<std::uint8_t>(data[1]) == 0x10;
    finish_probe(p, p->greeted);
}

void target_discovery::on_timeout(probe* p) {
    finish_probe(p, false);
}

void target_discovery::finish_probe(probe* p, bool open) {
    if (!probes_.removeOne(p)) {
        return;
    }
    if (p->timer) { p->timer->stop(); p->timer->deleteLater(); }
    if (p->sock)  { p->sock->abort(); p->sock->deleteLater(); }
    emit probe_checked(p->host, p->port, open);
    if (open) emit target_found(p->host, p->port, p->kind);
    delete p;
    maybe_emit_finished();
}

void target_discovery::maybe_emit_finished() {
    if (probes_.isEmpty()) emit scan_finished();
}

QList<QHostAddress> expand_cidr(const QString& cidr, QString* error) {
    auto set_err = [error](const QString& msg) {
        if (error) *error = msg;
    };
    if (error) error->clear();

    QHostAddress base;
    int prefix = 32;
    const int slash = cidr.indexOf('/');
    if (slash < 0) {
        if (!base.setAddress(cidr.trimmed())) {
            set_err(QStringLiteral("not a valid IP or CIDR: %1").arg(cidr));
            return {};
        }
    } else {
        base.setAddress(cidr.left(slash).trimmed());
        bool ok = false;
        prefix = cidr.mid(slash + 1).trimmed().toInt(&ok);
        if (!ok || prefix < 0 || prefix > 32) {
            set_err(QStringLiteral("bad CIDR suffix: %1").arg(cidr.mid(slash + 1)));
            return {};
        }
    }
    if (base.protocol() != QAbstractSocket::IPv4Protocol) {
        set_err(QStringLiteral("only IPv4 ranges are supported"));
        return {};
    }

    const quint32 raw    = base.toIPv4Address();
    const quint32 mask   = (prefix == 0) ? 0u : (0xffffffffu << (32 - prefix));
    const quint32 net    = raw & mask;
    const quint32 hosts  = (prefix >= 32) ? 1u : (~mask);

    QList<QHostAddress> out;
    if (prefix >= 31) {
        const quint32 count = (prefix == 32) ? 1u : 2u;
        for (quint32 i = 0; i < count; ++i) out.append(QHostAddress(net + i));
    } else {
        if (hosts > static_cast<quint32>(scan_host_cap)) {
            set_err(QStringLiteral("range too large (%1 hosts > %2 cap)").arg(hosts).arg(scan_host_cap));
            return {};
        }
        for (quint32 i = 1; i < hosts; ++i) out.append(QHostAddress(net + i));
    }
    return out;
}

} // namespace opentm::tm_ui
