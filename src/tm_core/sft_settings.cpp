#include "sft_settings.h"
#include "sft_schema.h"

#include <algorithm>

namespace opentm::tm_core {

namespace {

bool is_section_header(const QString& line, QString& name_out) {
    if (line.size() < 2) return false;
    if (!line.startsWith('[') || !line.endsWith(']')) return false;
    name_out = line.mid(1, line.size() - 2);
    return true;
}

} // namespace

sft_settings sft_settings::parse(const QByteArray& data) {
    sft_settings out;
    const QString text = QString::fromLatin1(data);

    section* current = nullptr;
    const auto lines = text.split(QLatin1Char('\n'));
    for (const auto& raw : lines) {
        QString line = raw;
        if (line.endsWith(QLatin1Char('\r'))) line.chop(1);
        line = line.trimmed();
        if (line.isEmpty()) continue;
        if (line.startsWith(QLatin1Char(';')) || line.startsWith(QLatin1Char('#')))
            continue;

        QString name;
        if (is_section_header(line, name)) {
            out.sections_.push_back(section{name, {}});
            current = &out.sections_.back();
            continue;
        }

        const int eq = line.indexOf(QLatin1Char('='));
        if (eq < 0) continue;
        if (!current) {
            out.sections_.push_back(section{QString(), {}});
            current = &out.sections_.back();
        }
        current->entries.push_back(entry{
            line.left(eq).trimmed(),
            line.mid(eq + 1)
        });
    }
    return out;
}

QByteArray sft_settings::serialise() const {
    QString out;
    out += QStringLiteral("\r\n");
    for (const auto& s : sections_) {
        out += QStringLiteral("[%1]\r\n").arg(s.name);
        for (const auto& e : s.entries) {
            out += QStringLiteral("%1=%2\r\n").arg(e.key, e.value);
        }
        out += QStringLiteral("\r\n");
    }
    return out.toLatin1();
}

const sft_settings::entry* sft_settings::find(const QString& sec, const QString& key) const {
    for (const auto& s : sections_) {
        if (s.name != sec) continue;
        for (const auto& e : s.entries) if (e.key == key) return &e;
    }
    return nullptr;
}

QString sft_settings::value(const QString& sec, const QString& key) const {
    const entry* e = find(sec, key);
    return e ? e->value : QString();
}

bool sft_settings::contains(const QString& sec, const QString& key) const {
    return find(sec, key) != nullptr;
}

void sft_settings::set(const QString& sec, const QString& key, const QString& val)
{
    for (auto& s : sections_) {
        if (s.name != sec) continue;
        for (auto& e : s.entries) {
            if (e.key == key) { e.value = val; return; }
        }
        s.entries.push_back(entry{key, val});
        return;
    }
    sections_.push_back(section{sec, {entry{key, val}}});
}

sft_settings sft_settings::diff_against(const sft_settings& baseline) const {
    sft_settings out;
    for (const auto& s : sections_) {
        for (const auto& e : s.entries) {
            if (baseline.contains(s.name, e.key)
                && baseline.value(s.name, e.key) == e.value) {
                continue;
            }
            out.set(s.name, e.key, e.value);
        }
    }
    return out;
}

sft_settings sft_settings::overrides_against(const sft_settings& baseline) const {
    const auto changed = diff_against(baseline);
    if (changed.empty()) return changed;

    sft_settings out;
    const QString ver = baseline.contains(QStringLiteral("Version"), QStringLiteral("version")) ? baseline.value(QStringLiteral("Version"), QStringLiteral("version")) : QStringLiteral("1");
    out.set(QStringLiteral("Version"), QStringLiteral("version"), ver);

    for (const auto& s : changed.sections()) {
        if (s.name == QLatin1String("Version")) continue;
        for (const auto& e : s.entries) out.set(s.name, e.key, e.value);

        for (const auto& k : coupled_keys(s.name)) {
            if (out.contains(s.name, k)) continue;
            if (contains(s.name, k))
            out.set(s.name, k, value(s.name, k));
            else if (baseline.contains(s.name, k))
                out.set(s.name, k, baseline.value(s.name, k));
        }
    }
    return out;
}

} // namespace opentm::tm_core
