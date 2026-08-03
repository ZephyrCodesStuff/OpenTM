#include "add_target_dialog.h"

#include <tm_core/wol.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace opentm::tm_ui {

using opentm::tm_core::target_type;
using opentm::tm_core::default_deci3_port;
using opentm::tm_core::target_type_string;

add_target_dialog::add_target_dialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Add Target"));
    setModal(true);

    auto* form = new QFormLayout;

    name_edit_ = new QLineEdit(this);
    name_edit_->setPlaceholderText(tr("e.g. devkit"));
    form->addRow(tr("&Name:"), name_edit_);

    type_combo_ = new QComboBox(this);
    type_combo_->addItem(QString::fromUtf8(target_type_string(target_type::decr_tcp).data()), static_cast<int>(target_type::decr_tcp));
    type_combo_->addItem(QString::fromUtf8(target_type_string(target_type::cfw_dex).data()), static_cast<int>(target_type::cfw_dex));
    type_combo_->addItem(QString::fromUtf8(target_type_string(target_type::core_dump).data()), static_cast<int>(target_type::core_dump));
    form->addRow(tr("&Target type:"), type_combo_);

    host_edit_ = new QLineEdit(this);
    host_edit_->setPlaceholderText(tr("192.0.2.10"));
    form->addRow(tr("&Host or IP:"), host_edit_);

    port_spin_ = new QSpinBox(this);
    port_spin_->setRange(1, 65535);
    port_spin_->setValue(default_deci3_port(target_type::decr_tcp));
    form->addRow(tr("&Port:"), port_spin_);

    mac_edit_ = new QLineEdit(this);
    mac_edit_->setPlaceholderText(tr("xx:xx:xx:xx:xx:xx (optional, for WoL)"));
    mac_edit_->setInputMask(tr(">HH:HH:HH:HH:HH:HH;_"));
    form->addRow(tr("&MAC:"), mac_edit_);

    auto make_dir_row = [this](QLineEdit** edit_out, const QString& placeholder) {
        auto* row = new QHBoxLayout;
        auto* edit = new QLineEdit(this);
        edit->setPlaceholderText(placeholder);
        auto* browse = new QPushButton(tr("Browse..."), this);
        row->addWidget(edit, 1);
        row->addWidget(browse, 0);
        row->setContentsMargins(0, 0, 0, 0);
        connect(browse, &QPushButton::clicked, this, [this, edit]() {
            const auto picked = QFileDialog::getExistingDirectory(
                this, tr("Select Directory"), edit->text());
            if (!picked.isEmpty()) edit->setText(picked);
        });
        *edit_out = edit;
        return row;
    };
    form->addRow(tr("File Server Dir (&app_home/):"), make_dir_row(&file_server_dir_edit_, tr("e.g. D:/builds/cellmark (served as /app_home/)")));
    form->addRow(tr("Home Dir (&~/):"), make_dir_row(&home_dir_edit_, tr("optional")));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(type_combo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &add_target_dialog::on_type_changed);
    connect(port_spin_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) { port_follows_type_ = false; });
}

void add_target_dialog::on_type_changed(int index) {
    if (!port_follows_type_) return;
    const auto t = static_cast<target_type>(type_combo_->itemData(index).toInt());
    const auto port = default_deci3_port(t);
    if (port != 0) {
        QSignalBlocker block(port_spin_);
        port_spin_->setValue(port);
    }
}

void add_target_dialog::set_record(const target_record& r) {
    base_ = r;
    name_edit_->setText(r.name);
    const int idx = type_combo_->findData(static_cast<int>(r.type));
    if (idx >= 0) type_combo_->setCurrentIndex(idx);
    host_edit_->setText(r.host);
    {
        QSignalBlocker block(port_spin_);
        port_spin_->setValue(r.port);
    }
    mac_edit_->setText(r.mac);
    file_server_dir_edit_->setText(r.file_server_dir);
    home_dir_edit_->setText(r.home_dir);
    // if port matches the type default, leave the follow type behavior on.
    port_follows_type_ = (r.port == default_deci3_port(r.type));
}

target_record add_target_dialog::record() const {
    target_record r = base_;
    r.name = name_edit_->text().trimmed();
    r.type = static_cast<target_type>(type_combo_->currentData().toInt());
    r.host = host_edit_->text().trimmed();
    r.port = static_cast<quint16>(port_spin_->value());
    // the input mask leaves '_' in unfilled slots, so a half-typed MAC would
    // otherwise be stored and only fail later at Wake on LAN
    r.mac = mac_edit_->text().trimmed();
    if (const auto parsed = opentm::tm_core::parse_mac(r.mac.toStdString())) {
        r.mac = QString::fromStdString(opentm::tm_core::format_mac(*parsed));
    } else {
        r.mac.clear();
    }
    r.file_server_dir = file_server_dir_edit_->text().trimmed();
    r.home_dir        = home_dir_edit_->text().trimmed();
    return r;
}

} // namespace opentm::tm_ui
