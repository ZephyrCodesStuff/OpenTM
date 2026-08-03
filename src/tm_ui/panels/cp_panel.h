#pragma once

#include <tm_core/cp_client.h>

#include <QWidget>
#include <QHash>
#include <QString>

class QPushButton;
class QLabel;
class QLineEdit;
class QRadioButton;

namespace opentm::tm_ui {

class cp_panel : public QWidget {
    Q_OBJECT
public:
    explicit cp_panel(QWidget* parent = nullptr);

    void set_host(const QString& host);

signals:
    void log_message(QString line);

private:
    void build_ui();
    void apply_host_state();
    void reload();
    void apply();
    void show_params(const opentm::tm_core::cp_boot_params& p);
    opentm::tm_core::cp_boot_params current_params() const;
    void set_busy(bool busy, const QString& what = {});

    opentm::tm_core::cp_client client_;
    QString host_;
    
    QHash<QString, QHash<QString, QRadioButton*>> buttons_;

    QLineEdit*        user_edit_ = nullptr;
    QLineEdit*        pass_edit_ = nullptr;
    QLabel*           status_    = nullptr;
    QPushButton*      reload_btn_ = nullptr;
    QPushButton*      apply_btn_  = nullptr;
};

} // namespace opentm::tm_ui
