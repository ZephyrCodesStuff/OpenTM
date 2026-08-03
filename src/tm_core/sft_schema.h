#pragma once

#include <QString>
#include <QStringList>

#include <vector>

namespace opentm::tm_core {

enum class sft_type {
    text,       // free form string
    integer,    // numeric, no named values
    boolean,    // 0/1 rendered as a checkbox
    choice,     // named values in 'choices'
};

struct sft_choice {
    QString value;
    QString label;
};

struct sft_field {
    QString                 group;
    QString                 label;
    sft_type                type = sft_type::text;
    std::vector<sft_choice> choices;

    bool control_only = false;
};

sft_field describe(const QString& section, const QString& key);
QStringList coupled_keys(const QString& section);

} // namespace opentm::tm_core
