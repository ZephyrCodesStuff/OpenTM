#pragma once

#include <QDateTime>
#include <QWidget>
#include <QString>

class QLineEdit;
class QPlainTextEdit;
class QPoint;

namespace opentm::tm_ui {

class find_dialog;

class wire_log_panel : public QWidget {
    Q_OBJECT
public:
    explicit wire_log_panel(QWidget* parent = nullptr);
    ~wire_log_panel() override;

public slots:
    void append(const QString& line);
    void clear();

public:
    void append_at(const QString& line, const QDateTime& when);
    void focus_find();
    QString text() const;

signals:
    void save_requested();

private:
    void show_context_menu(const QPoint& pos);

    QLineEdit*      filter_ = nullptr;
    QPlainTextEdit* view_   = nullptr;
    find_dialog*    find_   = nullptr;
    QString         last_line_;
    int             repeat_count_ = 0;
};

} // namespace opentm::tm_ui
