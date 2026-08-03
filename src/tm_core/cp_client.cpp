#include "cp_client.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

namespace opentm::tm_core {

namespace {

struct field {
    const char*  form_name;
    QString cp_boot_params::* member;
    const char*  allowed[4];
};

constexpr field kFields[] = {
    {"sysutil",  &cp_boot_params::boot_mode,     {"dbg", "sys", "rel", nullptr}},
    {"memmode",  &cp_boot_params::memory_size,   {"tool", "console", nullptr, nullptr}},
    {"bd",       &cp_boot_params::bd_access,     {"emu_dev", "emu_usb", "drive", nullptr}},
    {"hddspeed", &cp_boot_params::hdd_speed,     {"native", "emulated", nullptr, nullptr}},
    {"relchk",   &cp_boot_params::release_check, {"dev", "rel", nullptr, nullptr}},
    {"hostfs",   &cp_boot_params::hostfs,        {"dev", "target", nullptr, nullptr}},
    {"model",    &cp_boot_params::model,         {"ps3-hdd60", "ps3-hdd20", nullptr, nullptr}},
    {"bootbeep", &cp_boot_params::boot_beep,     {"beep", "silent", nullptr, nullptr}},
};

bool is_allowed(const field& f, const QString& value) {
    for (const char* a : f.allowed) {
        if (!a) break;
        if (value == QLatin1String(a)) return true;
    }
    return false;
}

QString describe(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
        return cp_client::tr("authentication failed, check the cp's user name and password");
    }
    return reply->errorString();
}

} // namespace

cp_client::cp_client(QObject* parent) : QObject(parent), net_(new QNetworkAccessManager(this)) {}

cp_client::~cp_client() = default;

void cp_client::set_credentials(const QString& user, const QString& password) {
    user_ = user;
    password_ = password;
}

QByteArray cp_client::auth_header() const {
    const auto raw = QStringLiteral("%1:%2").arg(user_, password_).toUtf8();
    return QByteArrayLiteral("Basic ") + raw.toBase64();
}

QString cp_client::page_url(const char* page) const {
    return QStringLiteral("http://%1/cgi-bin/%2?lang=english").arg(host_, QLatin1String(page));
}

bool cp_client::validate(const cp_boot_params& p, QString* bad_field) {
    for (const auto& f : kFields) {
        if (!is_allowed(f, p.*f.member)) {
            if (bad_field) *bad_field = QLatin1String(f.form_name);
            return false;
        }
    }
    return true;
}

void cp_client::fetch_boot_params(params_handler on_done) {
    if (host_.isEmpty()) {
        if (on_done) on_done(false, {}, tr("no cp host set"));
        return;
    }
    QNetworkRequest req{QUrl(page_url("user/be_param.cgi"))};
    req.setRawHeader("Authorization", auth_header());

    auto* reply = net_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, on_done] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (on_done) on_done(false, {}, describe(reply));
            return;
        }
        const auto html = QString::fromUtf8(reply->readAll());

        static const QRegularExpression re(QStringLiteral(R"RX(name="(\w+)"\s+value="([\w-]+)"([^>]*))RX"));
        cp_boot_params p;
        int found = 0;
        for (auto it = re.globalMatch(html); it.hasNext();) {
            const auto m = it.next();
            if (!m.captured(3).contains(QLatin1String("checked"), Qt::CaseInsensitive)) continue;
            for (const auto& f : kFields) {
                if (m.captured(1) != QLatin1String(f.form_name)) continue;
                p.*f.member = m.captured(2);
                ++found;
                break;
            }
        }
        if (found == 0) {
            if (on_done) on_done(false, {}, tr("could not read any settings from the cp page"));
            return;
        }
        emit log_message(tr("    -- cp: read %1/%2 boot parameters from %3").arg(found).arg(std::size(kFields)).arg(host_));
        if (on_done) on_done(true, p, {});
    });
}

void cp_client::apply_boot_params(const cp_boot_params& p, ack_handler on_done) {
    if (host_.isEmpty()) {
        if (on_done) on_done(false, tr("no cp host set"));
        return;
    }

    QString bad;
    if (!validate(p, &bad)) {
        if (on_done) on_done(false, tr("refusing to send an unrecognised value for '%1'").arg(bad));
        return;
    }

    QUrlQuery form;
    for (const auto& f : kFields) {
        form.addQueryItem(QLatin1String(f.form_name), p.*f.member);
    }

    QNetworkRequest req{QUrl(page_url("user/be_param.cgi"))};
    req.setRawHeader("Authorization", auth_header());
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));

    auto* reply = net_->post(req, form.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply, on_done] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (on_done) on_done(false, describe(reply));
            return;
        }
        emit log_message(tr("    -- cp: boot parameters written, they apply on the next boot"));
        if (on_done) on_done(true, {});
    });
}

} // namespace opentm::tm_core
