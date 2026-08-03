#include "arp_lookup.h"

#include <QFile>
#include <QHostAddress>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

namespace opentm::tm_core {

namespace {

#ifndef Q_OS_WIN
std::optional<mac_address> arp_from_command(const QString& ipv4) {
    const struct { const char* exe; QStringList args; } kProbes[] = {
        {"ip",  {QStringLiteral("neigh"), QStringLiteral("show"), ipv4}},
        {"arp", {QStringLiteral("-n"), ipv4}},
    };
    static const QRegularExpression mac_re(
        QStringLiteral("([0-9a-fA-F]{1,2}(?::[0-9a-fA-F]{1,2}){5})"));

    for (const auto& probe : kProbes) {
        QProcess p;
        p.start(QString::fromLatin1(probe.exe), probe.args);
        if (!p.waitForFinished(1500)) { p.kill(); continue; }
        const auto out = QString::fromLocal8Bit(p.readAllStandardOutput());
        const auto m = mac_re.match(out);
        if (!m.hasMatch()) continue;
        if (auto mac = parse_mac(m.captured(1).toStdString())) return mac;
    }
    return std::nullopt;
}
#endif

} // namespace

std::optional<mac_address> arp_lookup(const QString& ipv4) {
    QHostAddress addr;
    if (!addr.setAddress(ipv4) || addr.protocol() != QAbstractSocket::IPv4Protocol) {
        return std::nullopt;
    }

#ifdef Q_OS_WIN
    ULONG mac_buf[2] = {0, 0};
    ULONG mac_len = 6;
    const auto dest = htonl(addr.toIPv4Address());
    if (::SendARP(static_cast<IPAddr>(dest), 0, mac_buf, &mac_len) != NO_ERROR) {
        return std::nullopt;
    }
    if (mac_len != 6) return std::nullopt;

    const auto* bytes = reinterpret_cast<const std::uint8_t*>(mac_buf);
    mac_address mac{};
    for (std::size_t i = 0; i < mac.size(); ++i) mac[i] = bytes[i];
    // an all zero answer means "no entry", not a real address
    if (mac == mac_address{}) return std::nullopt;
    return mac;
#else
    return arp_from_command(ipv4);
#endif
}

} // namespace opentm::tm_core
