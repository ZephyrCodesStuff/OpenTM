#pragma once

#include <QDialog>

namespace opentm::tm_ui {

class about_dialog : public QDialog {
    Q_OBJECT
public:
    explicit about_dialog(QWidget* parent = nullptr);
};

} // namespace opentm::tm_ui