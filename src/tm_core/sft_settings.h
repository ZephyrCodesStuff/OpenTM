#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <utility>
#include <vector>

namespace opentm::tm_core {

class sft_settings {
public:
    struct entry {
        QString key;
        QString value;
    };
    struct section {
        QString            name;
        std::vector<entry> entries;
    };

    static sft_settings parse(const QByteArray& data);
    QByteArray serialise() const;
    QString value(const QString& section, const QString& key) const;
    bool    contains(const QString& section, const QString& key) const;
    void set(const QString& section, const QString& key, const QString& value);
    sft_settings diff_against(const sft_settings& baseline) const;
    sft_settings overrides_against(const sft_settings& baseline) const;

    const std::vector<section>& sections() const noexcept { return sections_; }
    bool empty() const noexcept { return sections_.empty(); }

private:
    const entry* find(const QString& section, const QString& key) const;

    std::vector<section> sections_;
};

} // namespace opentm::tm_core
