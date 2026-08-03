#include "discovery_dialog.h"

#include "app_prefs.h"

#include <tm_core/arp_lookup.h>
#include <tm_core/wol.h>

#include <tm_session/target_discovery.h>

#include <tm_core/target_type.h>

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace opentm::tm_ui {

namespace {

constexpr int col_pick    = 0;
constexpr int col_host    = 1;
constexpr int col_port    = 2;
constexpr int col_kind    = 3;
constexpr int col_name    = 4;
constexpr int col_mac     = 5;
constexpr int col_count   = 6;

QString kind_label(opentm::tm_core::target_type t) {
    switch (t) {
    case opentm::tm_core::target_type::decr_tcp:  return QStringLiteral("DECR");
    case opentm::tm_core::target_type::cfw_dex:   return QStringLiteral("Debugging Station / CFW DEX");
    case opentm::tm_core::target_type::core_dump: return QStringLiteral("Core");
    case opentm::tm_core::target_type::unknown:   break;
    }
    return QStringLiteral("?");
}

} // namespace

discovery_dialog::discovery_dialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Search for Targets"));
    setModal(true);
    resize(560, 380);

    auto* outer = new QVBoxLayout(this);

    auto* form = new QFormLayout;
    cidr_edit_ = new QLineEdit(this);
    {
        QSettings s;
        cidr_edit_->setText(s.value("discovery_cidr", "192.168.1.0/24").toString());
    }
    cidr_edit_->setPlaceholderText(tr("192.0.2.0/24 or 192.0.2.10"));
    form->addRow(tr("IP &range (CIDR or single):"), cidr_edit_);
    outer->addLayout(form);

    auto* row = new QHBoxLayout;
    scan_btn_ = new QPushButton(tr("&Scan"), this);
    scan_btn_->setDefault(true);
    progress_ = new QProgressBar(this);
    progress_->setRange(0, 1);
    progress_->setValue(0);
    progress_->setTextVisible(true);
    row->addWidget(scan_btn_);
    row->addWidget(progress_, 1);
    outer->addLayout(row);

    results_ = new QTableWidget(0, col_count, this);
    QStringList headers;
    headers << tr("") << tr("Host") << tr("Port") << tr("Type") << tr("Name") << tr("MAC");
    results_->setHorizontalHeaderLabels(headers);
    results_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    results_->horizontalHeader()->setStretchLastSection(true);
    results_->setColumnWidth(col_pick, 24);
    results_->setColumnWidth(col_host, 140);
    results_->setColumnWidth(col_port, 60);
    results_->setColumnWidth(col_kind, 80);
    results_->setColumnWidth(col_name, 110);
    results_->verticalHeader()->setVisible(false);
    results_->setSelectionBehavior(QAbstractItemView::SelectRows);
    outer->addWidget(results_, 1);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText(tr("Add &Selected"));
    outer->addWidget(btns);

    connect(scan_btn_, &QPushButton::clicked, this, &discovery_dialog::on_scan_clicked);
    connect(btns, &QDialogButtonBox::accepted, this, &discovery_dialog::on_accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    discovery_ = new target_discovery(this);
    connect(discovery_, &target_discovery::probe_checked, this, &discovery_dialog::on_probe_checked);
    connect(discovery_, &target_discovery::target_found, this, &discovery_dialog::on_target_found);
    connect(discovery_, &target_discovery::scan_finished, this, &discovery_dialog::on_scan_finished);
}

discovery_dialog::~discovery_dialog() = default;

void discovery_dialog::on_scan_clicked() {
    QString err;
    const auto hosts = expand_cidr(cidr_edit_->text(), &err);
    if (hosts.isEmpty()) {
        progress_->setFormat(err.isEmpty() ? tr("No hosts") : err);
        return;
    }
    QSettings s;
    s.setValue("discovery_cidr", cidr_edit_->text());

    results_->setRowCount(0);
    total_probes_  = hosts.size() * 2;
    checked_count_ = 0;
    progress_->setRange(0, total_probes_);
    progress_->setValue(0);
    progress_->setFormat(tr("Probing %v/%m..."));
    scan_btn_->setEnabled(false);
    discovery_->scan(hosts);
}

void discovery_dialog::on_probe_checked(QHostAddress /*host*/, std::uint16_t /*port*/, bool /*open*/)
{
    ++checked_count_;
    progress_->setValue(checked_count_);
}

void discovery_dialog::on_target_found(QHostAddress host, std::uint16_t port, opentm::tm_core::target_type kind)
{
    const int row = results_->rowCount();
    results_->insertRow(row);

    auto* pick = new QTableWidgetItem;
    pick->setFlags(pick->flags() | Qt::ItemIsUserCheckable);
    pick->setCheckState(Qt::Checked);
    results_->setItem(row, col_pick, pick);
    results_->setItem(row, col_host, new QTableWidgetItem(host.toString()));
    results_->setItem(row, col_port, new QTableWidgetItem(QString::number(port)));

    auto* kind_item = new QTableWidgetItem(kind_label(kind));

    kind_item->setData(Qt::UserRole, static_cast<int>(kind));
    results_->setItem(row, col_kind, kind_item);

    const auto last_octet = host.toString().section('.', -1);
    results_->setItem(row, col_name, new QTableWidgetItem(QStringLiteral("kit_%1").arg(last_octet)));

    QString mac_text;
    if (auto_arp_enabled()) {
        if (const auto mac = opentm::tm_core::arp_lookup(host.toString())) {
            mac_text = QString::fromStdString(opentm::tm_core::format_mac(*mac));
        }
    }
    results_->setItem(row, col_mac, new QTableWidgetItem(mac_text));
}

void discovery_dialog::on_scan_finished() {
    scan_btn_->setEnabled(true);
    progress_->setFormat(tr("Done - %1 kit(s)").arg(results_->rowCount()));
}

void discovery_dialog::on_accept() {
    picked_.clear();
    for (int row = 0; row < results_->rowCount(); ++row) {
        auto* pick = results_->item(row, col_pick);
        if (!pick || pick->checkState() != Qt::Checked) continue;
        target_record r;
        r.name = results_->item(row, col_name)->text().trimmed();
        if (r.name.isEmpty()) r.name = results_->item(row, col_host)->text();
        const auto kind_v = results_->item(row, col_kind)->data(Qt::UserRole).toInt();
        r.type = static_cast<opentm::tm_core::target_type>(kind_v);
        r.host = results_->item(row, col_host)->text();
        r.port = static_cast<quint16>(results_->item(row, col_port)->text().toUInt());
        if (auto* m = results_->item(row, col_mac)) r.mac = m->text();
        picked_.append(r);
    }
    accept();
}

} // namespace opentm::tm_ui
