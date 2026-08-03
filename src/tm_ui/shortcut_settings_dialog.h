#pragma once

#include <QDialog>
#include <QHash>
#include <QString>

class QAction;
class QKeySequenceEdit;
class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace opentm::tm_ui {

class shortcut_settings_dialog : public QDialog {
    Q_OBJECT
public:
    shortcut_settings_dialog(const QHash<QString, QAction*>& actions, QWidget* parent = nullptr);

    static void apply_saved(const QHash<QString, QAction*>& actions);
    static void save(const QString& key, const QKeySequence& seq);

private:
    void rebuild();
    void assign_from_editor();
    void clear_selected();
    void reset_selected();
    QTreeWidgetItem* selected() const;
    QString conflict_for(const QKeySequence& seq, const QString& except_key) const;

    QHash<QString, QAction*>     actions_;
    QHash<QString, QKeySequence> defaults_;
    QTreeWidget*                 tree_    = nullptr;
    QKeySequenceEdit*            editor_  = nullptr;
    QLabel*                      notice_  = nullptr;
};

} // namespace opentm::tm_ui
