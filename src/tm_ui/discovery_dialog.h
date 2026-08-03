#pragma once

#include "add_target_dialog.h"

#include <tm_core/target_type.h>

#include <QDialog>
#include <QHostAddress>
#include <QList>
#include <QString>

#include <cstdint>

class QLineEdit;
class QProgressBar;
class QPushButton;
class QTableWidget;

namespace opentm::tm_ui {

class target_discovery;

class discovery_dialog : public QDialog {
    Q_OBJECT
public:
    explicit discovery_dialog(QWidget* parent = nullptr);
    ~discovery_dialog() override;

    QList<target_record> selected_records() const { return picked_; }

private slots:
    void on_scan_clicked();
    void on_probe_checked(QHostAddress host, std::uint16_t port, bool open);
    void on_target_found(QHostAddress host, std::uint16_t port, opentm::tm_core::target_type kind);
    void on_scan_finished();
    void on_accept();

private:
    QLineEdit*        cidr_edit_  = nullptr;
    QPushButton*      scan_btn_   = nullptr;
    QProgressBar*     progress_   = nullptr;
    QTableWidget*     results_    = nullptr;
    target_discovery* discovery_  = nullptr;

    int                  total_probes_  = 0;
    int                  checked_count_ = 0;
    QList<target_record> picked_;
};

} // namespace opentm::tm_ui
