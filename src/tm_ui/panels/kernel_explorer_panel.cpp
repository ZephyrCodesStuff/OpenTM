#include "kernel_explorer_panel.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPair>
#include <QProxyStyle>
#include <QRegularExpression>
#include <QPushButton>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyleOption>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>

#include <optional>

namespace opentm::tm_ui {

namespace {

constexpr int kind_role     = Qt::UserRole + 1;
constexpr int pid_role      = Qt::UserRole + 2;
constexpr int category_role = Qt::UserRole + 3;
constexpr int payload_role  = Qt::UserRole + 4; // QVariant snapshot of the entry struct, used by detail pane

constexpr int kind_process  = 1;
constexpr int kind_category = 2;
constexpr int kind_entry    = 3;

const QString cat_ppu_threads = QStringLiteral("PPU Threads");
const QString cat_mutexes     = QStringLiteral("Mutexes");
const QString cat_lwmutexes   = QStringLiteral("Light Weight Mutexes");
const QString cat_conds       = QStringLiteral("Condition Variables");
const QString cat_event_queues = QStringLiteral("Event Queues");
const QString cat_modules     = QStringLiteral("Modules");
const QString cat_segments    = QStringLiteral("Segment List");
const QString cat_containers  = QStringLiteral("Memory Containers");

QIcon ico(const char* name) {
    return QIcon(QStringLiteral(":/icons/%1.bmp").arg(QLatin1String(name)));
}

QIcon icon_for_category(const QString& key) {
    static const QHash<QString, const char*> kMap = {
        {QStringLiteral("PPU Threads"),          "script"},
        {QStringLiteral("Mutexes"),              "lock"},
        {QStringLiteral("Light Weight Mutexes"), "lock_open"},
        {QStringLiteral("Condition Variables"),  "clock_go"},
        {QStringLiteral("Event Queues"),         "table_multiple"},
        {QStringLiteral("Modules"),              "bricks"},
        {QStringLiteral("Segment List"),         "page_white_stack"},
        {QStringLiteral("Memory Containers"),    "box"},
    };
    const auto it = kMap.constFind(key);
    return it == kMap.constEnd() ? QIcon() : ico(*it);
}

QString format_bytes(std::uint64_t b) {
    constexpr std::uint64_t kKB = 1024ull;
    constexpr std::uint64_t kMB = kKB * 1024ull;
    if (b >= kMB) return QStringLiteral("%1 MB").arg(static_cast<double>(b) / kMB, 0, 'f', 1);
    if (b >= kKB) return QStringLiteral("%1 KB").arg(static_cast<double>(b) / kKB, 0, 'f', 1);
    return QStringLiteral("%1 B").arg(b);
}

QString hex32(std::uint32_t v) {
    return QStringLiteral("0x%1").arg(v, 8, 16, QChar('0'));
}

QString seg_kind_name(std::uint64_t segment_num) {
    switch (segment_num) {
    case 0:  return QStringLiteral(".text");
    case 1:  return QStringLiteral(".data");
    default: return QStringLiteral(".rodata");
    }
}

QString fmt_seg_flags(std::uint64_t f) {
    auto perms = [](std::uint64_t bits) {
        QStringList out;
        if (bits & 0x1) out << QStringLiteral("EXEC");
        if (bits & 0x2) out << QStringLiteral("WRITE");
        if (bits & 0x4) out << QStringLiteral("READ");
        return out.isEmpty() ? QStringLiteral("NONE") : out.join(QLatin1Char('|'));
    };
    return QStringLiteral("0x%1 [ PPU (%2) SPU (%3) ]").arg(f, 0, 16).arg(perms(f & 0x7)).arg(perms((f >> 20) & 0x7));
}

QString hex64(std::uint64_t v) {
    return QStringLiteral("0x%1").arg(v, 16, 16, QChar('0'));
}

std::optional<double> sort_key_of(const QString& text) {
    static const QRegularExpression kHex(QStringLiteral("^0x([0-9a-fA-F]+)"));
    static const QRegularExpression kSize(QStringLiteral("^([0-9]+(?:\\.[0-9]+)?)\\s*(B|KB|MB|GB)\\b"));
    static const QRegularExpression kNum(QStringLiteral("^\\(?(-?[0-9]+)\\)?"));

    auto m = kHex.match(text);
    if (m.hasMatch()) return static_cast<double>(m.captured(1).toULongLong(nullptr, 16));

    m = kSize.match(text);
    if (m.hasMatch()) {
        double v = m.captured(1).toDouble();
        const QString unit = m.captured(2);
        if (unit == QLatin1String("KB")) v *= 1024.0;
        else if (unit == QLatin1String("MB")) v *= 1024.0 * 1024.0;
        else if (unit == QLatin1String("GB")) v *= 1024.0 * 1024.0 * 1024.0;
        return v;
    }

    m = kNum.match(text);
    if (m.hasMatch()) return m.captured(1).toDouble();
    return std::nullopt;
}

class sortable_item : public QTableWidgetItem {
public:
    explicit sortable_item(const QString& text) : QTableWidgetItem(text) {
        key_ = sort_key_of(text);
    }
    bool operator<(const QTableWidgetItem& other) const override {
        if (const auto* o = dynamic_cast<const sortable_item*>(&other)) {
            if (key_ && o->key_) return *key_ < *o->key_;
        }
        return text().compare(other.text(), Qt::CaseInsensitive) < 0;
    }

private:
    std::optional<double> key_;
};

QString lib_from_sync_name(const std::string& name) {
    if (name.empty()) return {};
    const QString s = QString::fromUtf8(name.c_str());
    struct { const char* prefix; const char* lib; } table[] = {
        // not sure if this is passed through the wire, so hardcode them for now... future TODO? return empty string if not found
        {"_lv2",     "liblv2.sprx"},
        {"_lc_",     "libc.a"},
        {"_lgcmtx",  "libgcc.a"},
        {"_gcmlwm",  "libgcm_pm.a"},
        {"_gcm",     "libgcm_pm.a"},
        {"_io_",     "libio.sprx"},
        {"_fs_",     "libfs.sprx"},
        {"_s__",     "libsysutil.sprx"},
        {"_smolwm",  "libsysmodule.sprx"},
        {"_df_",     "libdbgfont_gcm.a"},
    };
    for (const auto& e : table) {
        if (s.startsWith(QLatin1String(e.prefix))) {
            return QString::fromLatin1(e.lib);
        }
    }
    return {};
}

// tag a "Name (lib)" suffix iff we know the lib
QString name_with_lib(const std::string& raw_name) {
    const QString name = QString::fromUtf8(raw_name.c_str());
    const QString lib  = lib_from_sync_name(raw_name);
    if (lib.isEmpty()) return QStringLiteral("\"%1\"").arg(name);
    return QStringLiteral("\"%1\" (%2)").arg(name, lib);
}

class branch_arrow_style : public QProxyStyle { // hack to fix the chevron not appearing on certain qt stylesheets
public:
    using QProxyStyle::QProxyStyle;
    void drawPrimitive(PrimitiveElement el, const QStyleOption* opt, QPainter* p, const QWidget* w) const override
    {
        if (el == PE_IndicatorBranch
            && opt
            && (opt->state & State_Children))
        {
            const auto rect = opt->rect;
            const qreal cx = rect.x() + rect.width()  * 0.5;
            const qreal cy = rect.y() + rect.height() * 0.5;
            const qreal sz = std::min(rect.width(), rect.height()) * 0.32;
            p->save();
            p->setRenderHint(QPainter::Antialiasing);
            p->setPen(Qt::NoPen);
            const QColor fill = (opt->state & State_MouseOver) ? QColor("#e0e0e0") : QColor("#a8a8a8");
            p->setBrush(fill);
            QPolygonF tri;
            if (opt->state & State_Open) {
                // down chevron when expanded
                tri << QPointF(cx - sz, cy - sz * 0.5) << QPointF(cx + sz, cy - sz * 0.5) << QPointF(cx,      cy + sz * 0.7);
            } else {
                // right chevron when collapsed
                tri << QPointF(cx - sz * 0.5, cy - sz) << QPointF(cx - sz * 0.5, cy + sz) << QPointF(cx + sz * 0.7, cy);
            }
            p->drawPolygon(tri);
            p->restore();
            return;
        }
        QProxyStyle::drawPrimitive(el, opt, p, w);
    }
};

QStringList snapshot_expanded(QTreeView* tree, QStandardItemModel* model) {
    QStringList out;
    for (int i = 0; i < model->rowCount(); ++i) {
        auto* proc = model->item(i, 0);
        if (!proc) continue;
        const auto proc_idx = proc->index();
        if (tree->isExpanded(proc_idx)) {
            out.append(QStringLiteral("p:%1").arg(proc->data(pid_role).toUInt()));
        }
        for (int j = 0; j < proc->rowCount(); ++j) {
            auto* cat = proc->child(j, 0);
            if (!cat) continue;
            if (tree->isExpanded(cat->index())) {
                out.append(QStringLiteral("c:%1/%2").arg(proc->data(pid_role).toUInt()).arg(cat->data(category_role).toString()));
            }
        }
    }
    return out;
}

void restore_expanded(QTreeView* tree, QStandardItemModel* model, const QStringList& snapshot) {
    for (int i = 0; i < model->rowCount(); ++i) {
        auto* proc = model->item(i, 0);
        if (!proc) continue;
        const auto pid_key = QStringLiteral("p:%1").arg(
            proc->data(pid_role).toUInt());
        if (snapshot.contains(pid_key)) tree->expand(proc->index());
        for (int j = 0; j < proc->rowCount(); ++j) {
            auto* cat = proc->child(j, 0);
            if (!cat) continue;
            const auto cat_key = QStringLiteral("c:%1/%2")
                .arg(proc->data(pid_role).toUInt())
                .arg(cat->data(category_role).toString());
            if (snapshot.contains(cat_key)) tree->expand(cat->index());
        }
    }
}

} // namespace

kernel_explorer_panel::kernel_explorer_panel(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* tools = new QHBoxLayout;
    tools->setContentsMargins(6, 6, 6, 6);
    tools->setSpacing(4);

    refresh_btn_    = new QPushButton(tr("&Refresh"), this);
    copy_value_btn_ = new QPushButton(tr("Copy &Value"), this);
    copy_value_btn_->setToolTip(tr("Copy the selected detail-pane value to the clipboard"));
    core_dump_btn_  = new QPushButton(tr("Trigger Core &Dump"), this);
    core_dump_btn_->setToolTip(tr("DBGP_TRIGGER_CORE_DUMP on the selected process"));
    resume_btn_     = new QPushButton(tr("Resu&me"), this);
    resume_btn_->setToolTip(tr("DBGP_CONTINUE_PROCESS on the selected process"));
    pause_btn_      = new QPushButton(tr("&Pause"), this);
    pause_btn_->setToolTip(tr("DBGP_STOP_PROCESS on the selected process"));
    terminate_btn_  = new QPushButton(tr("&Terminate"), this);
    terminate_btn_->setToolTip(tr("DBGP_TERMINATE_GAME_PROCESS on the selected process"));

    status_label_ = new QLabel(tr("Idle. Click Refresh to populate."), this);
    status_label_->setStyleSheet(QStringLiteral("color:#888;"));

    tools->addWidget(refresh_btn_);
    refresh_btn_->setIcon(ico("arrow_refresh"));
    copy_value_btn_->setIcon(ico("page_white_copy"));
    core_dump_btn_->setIcon(ico("bug"));
    resume_btn_->setIcon(ico("control_play"));
    pause_btn_->setIcon(ico("control_pause_blue"));
    terminate_btn_->setIcon(ico("cross"));
    tools->addWidget(copy_value_btn_);
    tools->addWidget(core_dump_btn_);
    tools->addWidget(resume_btn_);
    tools->addWidget(pause_btn_);
    tools->addWidget(terminate_btn_);
    tools->addStretch(1);
    tools->addWidget(status_label_);
    outer->addLayout(tools);

    for (auto* b : { copy_value_btn_, core_dump_btn_, resume_btn_, pause_btn_, terminate_btn_ })
    {
        b->setEnabled(false);
    }
    auto* splitter = new QSplitter(Qt::Vertical, this);

    model_ = new QStandardItemModel(0, 1, this);
    model_->setHeaderData(0, Qt::Horizontal, tr("Kernel Object"));
    tree_ = new QTreeView(splitter);
    tree_->setModel(model_);
    tree_->setHeaderHidden(false);
    tree_->setUniformRowHeights(true);
    tree_->setAlternatingRowColors(true);
    tree_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree_->setIndentation(14);
    tree_->setStyleSheet(QStringLiteral("QTreeView::item { padding: 0px; margin: 0px; }"));
    auto* arrow_style = new branch_arrow_style;
    arrow_style->setParent(tree_);
    tree_->setStyle(arrow_style);
    splitter->addWidget(tree_);

    detail_ = new QTableWidget(0, 2, splitter);
    detail_->setHorizontalHeaderLabels({tr("Attribute"), tr("Value")});
    detail_->horizontalHeader()->setStretchLastSection(true);
    detail_->verticalHeader()->setVisible(false);
    detail_->setSelectionBehavior(QAbstractItemView::SelectRows);
    detail_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    detail_->setContextMenuPolicy(Qt::CustomContextMenu);
    detail_->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    detail_->verticalHeader()->setDefaultSectionSize(detail_->fontMetrics().height() + 4);
    detail_->setShowGrid(true);
    detail_->setAlternatingRowColors(true);
    splitter->addWidget(detail_);
    splitter->setStretchFactor(0, 5);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({600, 120});

    outer->addWidget(splitter, 1);

    connect(refresh_btn_, &QPushButton::clicked, this, &kernel_explorer_panel::on_refresh_clicked);
    connect(tree_, &QTreeView::expanded, this, &kernel_explorer_panel::on_tree_expanded);
    connect(tree_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &kernel_explorer_panel::on_tree_selection_changed);

    connect(copy_value_btn_, &QPushButton::clicked, this, [this]() {
        const auto rows = detail_->selectionModel()->selectedRows();
        QStringList vals;
        for (const auto& r : rows) {
            if (auto* it = detail_->item(r.row(), 1)) vals.append(it->text());
        }
        if (vals.isEmpty()) return;
        QApplication::clipboard()->setText(vals.join('\n'));
        status_label_->setText(tr("Copied: %1").arg(vals.join(QStringLiteral(", "))));
    });

    connect(detail_, &QTableWidget::customContextMenuRequested,
            this, [this](const QPoint& pos) {
        const auto rows = detail_->selectionModel()->selectedRows();
        if (rows.isEmpty()) return;
        auto selection_text = [this](bool with_headers) {
            QStringList lines;
            if (with_headers) {
                QStringList hdr;
                for (int c = 0; c < detail_->columnCount(); ++c) {
                    hdr << detail_->horizontalHeaderItem(c)->text();
                }
                lines << hdr.join(QLatin1Char('\t'));
            }
            for (const auto& r : detail_->selectionModel()->selectedRows()) {
                QStringList cells;
                for (int c = 0; c < detail_->columnCount(); ++c) {
                    auto* it = detail_->item(r.row(), c);
                    cells << (it ? it->text() : QString());
                }
                lines << cells.join(QLatin1Char('\t'));
            }
            return lines.join(QLatin1Char('\n'));
        };

        QMenu menu(this);
        auto* copy_cell = menu.addAction(tr("Copy"));
        auto* copy_row  = menu.addAction(tr("Copy Row"));
        auto* copy_hdr  = menu.addAction(tr("Copy with Headers"));
        menu.addSeparator();
        auto* copy_all  = menu.addAction(tr("Copy All Rows"));

        QAction* chosen = menu.exec(detail_->viewport()->mapToGlobal(pos));
        if (!chosen) return;

        if (chosen == copy_cell) {
            const auto idx = detail_->indexAt(pos);
            if (auto* it = detail_->item(idx.row(), idx.column())) {
                QApplication::clipboard()->setText(it->text());
            }
        } else if (chosen == copy_row) {
            QApplication::clipboard()->setText(selection_text(false));
        } else if (chosen == copy_hdr) {
            QApplication::clipboard()->setText(selection_text(true));
        } else if (chosen == copy_all) {
            detail_->selectAll();
            QApplication::clipboard()->setText(selection_text(true));
        }
        status_label_->setText(tr("Copied to clipboard"));
    });
    connect(core_dump_btn_, &QPushButton::clicked, this, [this]() {
        const auto pid = current_process_pid();
        if (pid) emit core_dump_requested(pid);
    });
    connect(resume_btn_, &QPushButton::clicked, this, [this]() {
        const auto pid = current_process_pid();
        if (pid) emit resume_requested(pid);
    });
    connect(pause_btn_, &QPushButton::clicked, this, [this]() {
        const auto pid = current_process_pid();
        if (pid) emit pause_requested(pid);
    });
    connect(terminate_btn_, &QPushButton::clicked, this, [this]() {
        const auto pid = current_process_pid();
        if (!pid) return;
        const auto reply = QMessageBox::question(this, tr("Terminate Process"), tr("Send DBGP_TERMINATE_GAME_PROCESS to pid 0x%1?").arg(pid, 8, 16, QChar('0')), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (reply == QMessageBox::Yes) emit terminate_requested(pid);
    });
}

std::uint32_t kernel_explorer_panel::current_process_pid() const {
    const auto idx = tree_->selectionModel()->currentIndex();
    if (!idx.isValid()) return 0;
    auto* item = model_->itemFromIndex(idx);
    while (item) {
        if (item->data(kind_role).toInt() == kind_process) {
            return static_cast<std::uint32_t>(item->data(pid_role).toUInt());
        }
        item = item->parent();
    }
    return 0;
}

kernel_explorer_panel::~kernel_explorer_panel() = default;

void kernel_explorer_panel::on_refresh_clicked() {
    status_label_->setText(tr("Fetching process list..."));
    emit refresh_requested();
}

void kernel_explorer_panel::on_tree_expanded(const QModelIndex& idx) {
    if (!idx.isValid()) return;
    auto* item = model_->itemFromIndex(idx);
    if (!item) return;
    if (item->data(kind_role).toInt() != kind_category) return;

    if (item->rowCount() == 1) {
        auto* child = item->child(0, 0);
        if (child && child->text() == QStringLiteral("...")) {
            const auto pid = static_cast<std::uint32_t>(item->data(pid_role).toUInt());
            const auto key = item->data(category_role).toString();
            if      (key == cat_ppu_threads)   emit threads_requested(pid);
            else if (key == cat_mutexes)       emit mutexes_requested(pid);
            else if (key == cat_lwmutexes)     emit lwmutexes_requested(pid);
            else if (key == cat_conds)         emit conds_requested(pid);
            else if (key == cat_event_queues)  emit event_queues_requested(pid);
            else if (key == cat_modules)       emit modules_requested(pid);
            else if (key == cat_containers)    emit containers_requested(pid);
        }
    }
}

QStandardItem* kernel_explorer_panel::find_or_create_category(
    QStandardItem* process_item, const QString& key)
{
    if (!process_item) return nullptr;
    for (int i = 0; i < process_item->rowCount(); ++i) {
        auto* c = process_item->child(i, 0);
        if (c && c->data(category_role).toString() == key) return c;
    }
    auto* c = new QStandardItem(key);
    c->setEditable(false);
    c->setIcon(icon_for_category(key));
    c->setData(kind_category, kind_role);
    c->setData(key, category_role);
    c->setData(process_item->data(pid_role), pid_role);
    auto* placeholder = new QStandardItem(QStringLiteral("..."));
    placeholder->setEditable(false);
    placeholder->setSelectable(false);
    c->appendRow(placeholder);
    process_item->appendRow(c);
    return c;
}

void kernel_explorer_panel::clear() {
    model_->removeRows(0, model_->rowCount());
    detail_->setRowCount(0);
    threads_cache_.clear();
    mutexes_cache_.clear();
    lwmutexes_cache_.clear();
    conds_cache_.clear();
    evqs_cache_.clear();
    modules_cache_.clear();
    containers_cache_.clear();
    status_label_->setText(tr("Target disconnected."));
}

void kernel_explorer_panel::on_process_list_ready(
    QList<session_api::process_summary> processes)
{
    const auto expanded_snapshot = snapshot_expanded(tree_, model_);

    model_->removeRows(0, model_->rowCount());
    if (processes.isEmpty()) {
        auto* placeholder = new QStandardItem(tr("(no process loaded, it may not be a debug EBOOT)"));
        placeholder->setEditable(false);
        placeholder->setSelectable(false);
        QFont f = placeholder->font();
        f.setItalic(true);
        placeholder->setFont(f);
        model_->appendRow(placeholder);
        status_label_->setText(tr("No running processes."));
        return;
    }
    status_label_->setText(tr("%1 process(es). Expand to drill down.").arg(processes.size()));
    for (const auto& p : processes) {
        const auto title = QStringLiteral("Process, ID = %1, Total Memory = %2").arg(hex32(p.pid)).arg(format_bytes(static_cast<std::uint64_t>(p.memory.local_memory) + p.memory.local_text + p.memory.prx_text + p.memory.prx_data   + p.memory.remain_memory));
        auto* root = new QStandardItem(title);
        root->setEditable(false);
        root->setData(kind_process, kind_role);
        root->setData(p.pid, pid_role);
        // stash the whole summary for the detail pane.
        root->setData(QVariant::fromValue(p.info.self_path.empty() ? QString() : QString::fromUtf8(p.info.self_path.c_str())), payload_role);
        model_->appendRow(root);
        for (const QString& key : {cat_ppu_threads, cat_mutexes, cat_lwmutexes, cat_conds, cat_event_queues, cat_modules, cat_containers})
        {
            find_or_create_category(root, key);
        }
        tree_->expand(root->index());
    }
    restore_expanded(tree_, model_, expanded_snapshot);

    for (const auto& p : processes) {
        emit threads_requested(p.pid);
        emit mutexes_requested(p.pid);
        emit lwmutexes_requested(p.pid);
        emit conds_requested(p.pid);
        emit event_queues_requested(p.pid);
        emit modules_requested(p.pid);
        emit containers_requested(p.pid);
    }
}
namespace {

QStandardItem* category_node_for(QStandardItemModel* model, std::uint32_t pid, const QString& key)
{
    for (int i = 0; i < model->rowCount(); ++i) {
        auto* proc = model->item(i, 0);
        if (!proc) continue;
        if (static_cast<std::uint32_t>(proc->data(pid_role).toUInt()) != pid) continue;
        for (int j = 0; j < proc->rowCount(); ++j) {
            auto* cat = proc->child(j, 0);
            if (cat && cat->data(category_role).toString() == key) return cat;
        }
    }
    return nullptr;
}

QString thread_label(const opentm::tm_core::dbgp::ppu_thread_info& t, const QString& suffix) {
    return QStringLiteral("Thread: ID = %1 \"%2\", 0x%3 - %4, %5").arg(hex32(static_cast<std::uint32_t>(t.thread_id))).arg(QString::fromUtf8(t.name.c_str())).arg(t.state, 2, 16, QChar('0')).arg(QString::fromUtf8(opentm::tm_core::dbgp::ppu_thread_state_name(t.state))).arg(suffix);
}

} // namespace

namespace {

struct thread_render {
    static QString summary(const opentm::tm_core::dbgp::ppu_thread_info& e) {
        return QStringLiteral("Thread ID = %1 \"%2\", %3").arg(hex32(static_cast<std::uint32_t>(e.thread_id))).arg(QString::fromUtf8(e.name.c_str())).arg(opentm::tm_core::dbgp::ppu_thread_state_name(e.state));
    }
};

struct mutex_render {
    static QString summary(const opentm::tm_core::dbgp::mutex_info& e) {
        QString out = QStringLiteral("Mutex ID = %1 %2").arg(hex32(e.handle)).arg(name_with_lib(e.name));
        if (e.owner_thread_id != 0) {
            out += QStringLiteral(", Owner Thread ID = %1").arg(hex64(e.owner_thread_id));
        }
        return out;
    }
};

struct lwmutex_render {
    static QString summary(const opentm::tm_core::dbgp::lwmutex_info& e) {
        const auto lib = lib_from_sync_name(e.name);
        QString out;
        if (!e.name.empty()) {
            out = QStringLiteral("LW Mutex ID = %1 %2").arg(hex32(e.handle)).arg(name_with_lib(e.name));
        } else {
            out = QStringLiteral("LW Mutex ID = %1").arg(hex32(e.handle));
        }
        if (e.owner_thread_id == 0xFFFFFFFFu) {
            out += QStringLiteral(", Owner Thread ID = 0xFFFFFFFF - INITIALIZED");
        } else if (e.owner_thread_id != 0) {
            out += QStringLiteral(", Owner Thread ID = %1").arg(hex32(e.owner_thread_id));
        }
        return out;
    }
};

struct cond_render {
    static QString summary(const opentm::tm_core::dbgp::cond_info& e) {
        return QStringLiteral("Condition Variable ID = %1 %2")
            .arg(hex32(e.handle))
            .arg(name_with_lib(e.name));
    }
};

struct evq_render {
    static QString summary(const opentm::tm_core::dbgp::event_queue_info& e) {
        return QStringLiteral("Event Queue: I, Queued Events = %1").arg(e.queued_count);
    }
};

struct container_render {
    static QString summary(const opentm::tm_core::dbgp::container_info& e) {
        return QStringLiteral("Container ID = %1, Total = %2, Available = %3").arg(hex32(e.id)).arg(format_bytes(e.total)).arg(format_bytes(e.available));
    }
};

template <class Entry, class Render>
void replace_category(QStandardItemModel* model, std::uint32_t pid, const QString& key, const QList<Entry>& entries)
{
    QStandardItem* cat = category_node_for(model, pid, key);
    if (!cat) return;
    cat->removeRows(0, cat->rowCount());
    cat->setText(QStringLiteral("%1 (%2)").arg(key).arg(entries.size()));
    for (const auto& e : entries) {
        auto* it = new QStandardItem(Render::summary(e));
        it->setEditable(false);
        it->setData(kind_entry, kind_role);
        it->setData(pid, pid_role);
        it->setData(key, category_role);
        QVariantList attrs;
        auto add = [&](const QString& k, const QString& v) {
            attrs.append(QVariantMap{{"k", k}, {"v", v}});
        };
        if constexpr (std::is_same_v<Entry, opentm::tm_core::dbgp::ppu_thread_info>) {
            add(QStringLiteral("Thread ID"),     hex64(e.thread_id));
            add(QStringLiteral("Priority"),      QString::number(e.priority));
            add(QStringLiteral("State"),         QString::asprintf("%u (%s)", e.state, opentm::tm_core::dbgp::ppu_thread_state_name(e.state)));
            add(QStringLiteral("Stack Address"), hex64(e.stack_addr));
            add(QStringLiteral("Stack Size"),    format_bytes(e.stack_size));
            add(QStringLiteral("Base Priority"), QString::number(e.base_priority));
            add(QStringLiteral("Name"),          QString::fromUtf8(e.name.c_str()));
        } else if constexpr (std::is_same_v<Entry, opentm::tm_core::dbgp::mutex_info>) {
            add(QStringLiteral("Handle"),         hex32(e.handle));
            add(QStringLiteral("Protocol"),  hex32(e.attr_protocol));
            add(QStringLiteral("Recursive"), hex32(e.attr_recursive));
            add(QStringLiteral("Attr Shared"),    hex32(e.attr_shared));
            add(QStringLiteral("Adaptive"),  hex32(e.attr_adaptive));
            add(QStringLiteral("Key"),            hex64(e.key));
            add(QStringLiteral("Flags"),          hex32(e.flags));
            add(QStringLiteral("Name"),           QString::fromUtf8(e.name.c_str()));
            add(QStringLiteral("Owner Thread"),   hex64(e.owner_thread_id));
            add(QStringLiteral("Lock Count"),     QString::number(e.lock_counter));
            add(QStringLiteral("CV Ref Count"),   QString::number(e.cond_ref_counter));
            add(QStringLiteral("CV ID"),          hex32(e.cond_var_id));
            add(QStringLiteral("Waiters"),        QString::number(e.wait_thread_count));
        } else if constexpr (std::is_same_v<Entry, opentm::tm_core::dbgp::lwmutex_info>) {
            add(QStringLiteral("Handle"),        hex32(e.handle));
            add(QStringLiteral("Protocol"), hex32(e.attr_protocol));
            add(QStringLiteral("Recursive"),hex32(e.attr_recursive));
            add(QStringLiteral("Name"),          QString::fromUtf8(e.name.c_str()));
            add(QStringLiteral("Owner Thread"),  hex32(e.owner_thread_id));
            add(QStringLiteral("Lock Count"),    QString::number(e.lock_counter));
            add(QStringLiteral("Waiters"),       QString::number(e.wait_thread_ids.size()));
        } else if constexpr (std::is_same_v<Entry, opentm::tm_core::dbgp::cond_info>) {
            add(QStringLiteral("Handle"),     hex32(e.handle));
            add(QStringLiteral("Mutex ID"),   hex32(e.mutex_id));
            add(QStringLiteral("Attr Shared"),hex32(e.attr_shared));
            add(QStringLiteral("Key"),        hex64(e.key));
            add(QStringLiteral("Flags"),      hex32(e.flags));
            add(QStringLiteral("Name"),       QString::fromUtf8(e.name.c_str()));
            add(QStringLiteral("Waiters"),    QString::number(e.wait_thread_ids.size()));
        } else if constexpr (std::is_same_v<Entry, opentm::tm_core::dbgp::event_queue_info>) {
            add(QStringLiteral("Handle"),       hex32(e.handle));
            add(QStringLiteral("Protocol"),hex32(e.attr_protocol));
            add(QStringLiteral("Type"),         hex32(e.type));
            add(QStringLiteral("Key"),          hex64(e.key));
            add(QStringLiteral("Queue Size"),   QString::number(e.queue_size));
            add(QStringLiteral("Name"),         QString::fromUtf8(e.name.c_str()));
            add(QStringLiteral("Queued"),       QString::number(e.queued_count));
            add(QStringLiteral("Waiters"),      QString::number(e.wait_thread_ids.size()));
        } else if constexpr (std::is_same_v<Entry, opentm::tm_core::dbgp::container_info>) {
            add(QStringLiteral("ID"),        hex32(e.id));
            add(QStringLiteral("Parent ID"), e.parent_id == 0xFFFFFFFFu ? QStringLiteral("(none)") : hex32(e.parent_id));
            add(QStringLiteral("Total"),     format_bytes(e.total));
            add(QStringLiteral("Available"), format_bytes(e.available));
        }
        it->setData(attrs, payload_role);
        cat->appendRow(it);
    }
}

} // namespace

void kernel_explorer_panel::on_threads_ready(
    std::uint32_t pid,
    QList<opentm::tm_core::dbgp::ppu_thread_info> threads)
{
    threads_cache_[pid] = threads;
    replace_category<opentm::tm_core::dbgp::ppu_thread_info, thread_render>( model_, pid, cat_ppu_threads, threads);

    auto* cat = category_node_for(model_, pid, cat_ppu_threads);
    if (!cat) return;
    const auto& mutexes = mutexes_cache_.value(pid);
    const auto& evqs    = evqs_cache_.value(pid);
    for (int i = 0; i < cat->rowCount(); ++i) {
        auto* trow = cat->child(i, 0);
        if (!trow) continue;
        if (i >= threads.size()) break;
        const auto tid = threads[i].thread_id;
        for (const auto& m : mutexes) {
            if (m.owner_thread_id == tid) {
                auto* mrow = new QStandardItem(QStringLiteral("Mutex: ID = %1 %2, Owner Thread ID = %3, Owned By").arg(hex32(m.handle)).arg(name_with_lib(m.name)).arg(hex64(m.owner_thread_id)));
                mrow->setEditable(false);
                mrow->setData(kind_entry, kind_role);
                trow->appendRow(mrow);
            }
        }
        for (const auto& q : evqs) {
            for (auto wt : q.wait_thread_ids) {
                if (wt == tid) {
                    auto* qrow = new QStandardItem(QStringLiteral("Event Queue: I, Queued Events = %1, Waiting").arg(q.queued_count));
                    qrow->setEditable(false);
                    qrow->setData(kind_entry, kind_role);
                    trow->appendRow(qrow);
                    break;
                }
            }
        }
    }
}

void kernel_explorer_panel::on_modules_ready(
    std::uint32_t pid,
    QList<opentm::tm_core::dbgp::prx_info> modules)
{
    modules_cache_[pid] = modules;
    auto* cat = category_node_for(model_, pid, cat_modules);
    if (!cat) return;
    cat->removeRows(0, cat->rowCount());

    std::uint64_t total_mem = 0;
    std::uint64_t text_mem  = 0;
    std::uint64_t data_mem  = 0;
    int seg_total = 0;
    for (const auto& m : modules) {
        total_mem += m.mem_size;
        seg_total += static_cast<int>(m.segments.size());
        for (const auto& s : m.segments) {
            if      (s.segment_num == 0) text_mem += s.mem_size;
            else if (s.segment_num == 1) data_mem += s.mem_size;
        }
    }
    cat->setText(QStringLiteral("%1 (%2), Mem Size = %3 (.text = %4, .data = %5)").arg(cat_modules).arg(modules.size()).arg(format_bytes(total_mem)).arg(format_bytes(text_mem)).arg(format_bytes(data_mem)));

    {
        auto* seglist = new QStandardItem(QStringLiteral("Segment List (%1)").arg(seg_total));
        seglist->setEditable(false);
        seglist->setIcon(ico("page_white_stack"));
        seglist->setData(kind_category, kind_role);
        seglist->setData(pid, pid_role);
        seglist->setData(cat_segments, category_role);
        cat->appendRow(seglist);
    }

    for (const auto& m : modules) {
        const auto summary = QStringLiteral("PRX: %1, Mem Size = %2").arg(QString::fromUtf8(m.name.c_str())).arg(format_bytes(m.mem_size));
        auto* row = new QStandardItem(summary);
        row->setEditable(false);
        row->setData(kind_entry, kind_role);
        row->setData(pid, pid_role);
        row->setData(cat_modules, category_role);

        QVariantList attrs;
        auto add = [&](const QString& k, const QString& v) {
            attrs.append(QVariantMap{{"k", k}, {"v", v}});
        };
        add(QStringLiteral("Module ID"),    hex32(m.handle));
        add(QStringLiteral("Name"),         QString::fromUtf8(m.name.c_str()));
        add(QStringLiteral("Mem Size"),     QStringLiteral("0x%1 (%2)").arg(m.mem_size, 0, 16).arg(format_bytes(m.mem_size)));
        add(QStringLiteral("Version"),      QStringLiteral("%1.%2").arg(m.version_major).arg(m.version_minor));
        add(QStringLiteral("Attribute"),    hex32(m.attribute));
        add(QStringLiteral("Start"),        hex32(m.start_entry));
        add(QStringLiteral("Stop"),         hex32(m.stop_entry));
        add(QStringLiteral("PRX Name"),     QString::fromUtf8(m.path.c_str()));
        add(QStringLiteral("Segments"),     QString::number(m.segments.size()));
        row->setData(attrs, payload_role);

        for (std::size_t s = 0; s < m.segments.size(); ++s) {
            const auto& seg = m.segments[s];
            QString kind;
            switch (seg.segment_num) {
            case 0:  kind = QStringLiteral(".text"); break;
            case 1:  kind = QStringLiteral(".data"); break;
            default: kind = QStringLiteral("seg%1").arg(seg.segment_num); break;
            }
            auto* segrow = new QStandardItem(QStringLiteral("Segment: Mem Size (%1) = 0x%2 (%3)").arg(kind).arg(seg.mem_size, 0, 16).arg(format_bytes(seg.mem_size)));
            segrow->setEditable(false);
            segrow->setData(kind_entry, kind_role);

            QVariantList seg_attrs;
            auto add_seg = [&](const QString& k, const QString& v) {
                seg_attrs.append(QVariantMap{{"k", k}, {"v", v}});
            };
            add_seg(QStringLiteral("Kind"),         kind);
            add_seg(QStringLiteral("Base"),         hex64(seg.base));
            add_seg(QStringLiteral("File Size"),    format_bytes(seg.file_size));
            add_seg(QStringLiteral("Mem Size"),     format_bytes(seg.mem_size));
            add_seg(QStringLiteral("Segment Num"),  QString::number(seg.segment_num));
            add_seg(QStringLiteral("Segment Type"), hex64(seg.segment_type));
            add_seg(QStringLiteral("Attr Flags"),   hex64(seg.attr_flags));
            add_seg(QStringLiteral("Align"),        QString::number(seg.align));
            segrow->setData(seg_attrs, payload_role);

            row->appendRow(segrow);
        }
        cat->appendRow(row);
    }
}

void kernel_explorer_panel::on_mutexes_ready(
    std::uint32_t pid,
    QList<opentm::tm_core::dbgp::mutex_info> entries)
{
    mutexes_cache_[pid] = entries;
    replace_category<opentm::tm_core::dbgp::mutex_info, mutex_render>(model_, pid, cat_mutexes, entries);

    auto* cat = category_node_for(model_, pid, cat_mutexes);
    if (!cat) return;
    const auto& threads = threads_cache_.value(pid);
    for (int i = 0; i < cat->rowCount() && i < entries.size(); ++i) {
        const auto& m = entries[i];
        if (m.owner_thread_id == 0) continue;
        auto* mrow = cat->child(i, 0);
        if (!mrow) continue;
        for (const auto& t : threads) {
            if (t.thread_id == m.owner_thread_id) {
                auto* trow = new QStandardItem(thread_label(t, QStringLiteral("Owns...")));
                trow->setEditable(false);
                trow->setData(kind_entry, kind_role);
                mrow->appendRow(trow);
                break;
            }
        }
    }
}

void kernel_explorer_panel::on_lwmutexes_ready(
    std::uint32_t pid,
    QList<opentm::tm_core::dbgp::lwmutex_info> entries)
{
    lwmutexes_cache_[pid] = entries;
    replace_category<opentm::tm_core::dbgp::lwmutex_info, lwmutex_render>(model_, pid, cat_lwmutexes, entries);
}

void kernel_explorer_panel::on_conds_ready(
    std::uint32_t pid,
    QList<opentm::tm_core::dbgp::cond_info> entries)
{
    conds_cache_[pid] = entries;
    replace_category<opentm::tm_core::dbgp::cond_info, cond_render>(model_, pid, cat_conds, entries);
}

void kernel_explorer_panel::on_event_queues_ready(
    std::uint32_t pid,
    QList<opentm::tm_core::dbgp::event_queue_info> entries)
{
    evqs_cache_[pid] = entries;
    replace_category<opentm::tm_core::dbgp::event_queue_info, evq_render>(model_, pid, cat_event_queues, entries);

    auto* cat = category_node_for(model_, pid, cat_event_queues);
    if (!cat) return;
    const auto& threads = threads_cache_.value(pid);
    for (int i = 0; i < cat->rowCount() && i < entries.size(); ++i) {
        const auto& q = entries[i];
        auto* qrow = cat->child(i, 0);
        if (!qrow) continue;
        for (const auto wt : q.wait_thread_ids) {
            // find the matching thread, fall back to raw ID if unknown.
            const opentm::tm_core::dbgp::ppu_thread_info* found = nullptr;
            for (const auto& t : threads) if (t.thread_id == wt) { found = &t; break; }
            const QString label = found ? thread_label(*found, QStringLiteral("Waiting")) : QStringLiteral("Thread: ID = %1, Waiting").arg(hex64(wt));
            auto* trow = new QStandardItem(label);
            trow->setEditable(false);
            trow->setData(kind_entry, kind_role);
            qrow->appendRow(trow);
        }
        auto* events = new QStandardItem(QStringLiteral("Events (%1)").arg(q.queued_count));
        events->setEditable(false);
        events->setSelectable(false);
        qrow->appendRow(events);
    }
}

void kernel_explorer_panel::on_containers_ready(
    std::uint32_t pid,
    QList<opentm::tm_core::dbgp::container_info> entries)
{
    containers_cache_[pid] = entries;
    replace_category<opentm::tm_core::dbgp::container_info, container_render>(model_, pid, cat_containers, entries);
}

namespace {

QString hex_with_name(std::uint32_t v, int width, const char* name) {
    return name ? QStringLiteral("0x%1 (%2)").arg(v, width, 16, QChar('0')).arg(QLatin1String(name)) : QStringLiteral("0x%1").arg(v, width, 16, QChar('0'));
}
QString fmt_protocol(std::uint32_t v) {
    const char* n = v == 0x00001 ? "SYS_SYNC_FIFO" : v == 0x00002 ? "SYS_SYNC_PRIORITY" : v == 0x00004 ? "SYS_SYNC_PRIORITY_INHERIT" : nullptr;
    return hex_with_name(v, 5, n);
}
QString fmt_recursive(std::uint32_t v) {
    const char* n = v == 0x00010 ? "SYS_SYNC_RECURSIVE" : v == 0x00020 ? "SYS_SYNC_NOT_RECURSIVE" : nullptr;
    return hex_with_name(v, 5, n);
}
QString fmt_pshared(std::uint32_t v) {
    return hex_with_name(v, 5, v == 0x00200 ? "SYS_SYNC_NOT_PROCESS_SHARED" : nullptr);
}
QString fmt_adaptive(std::uint32_t v) {
    return hex_with_name(v, 5, v == 0x02000 ? "SYS_SYNC_NOT_ADAPTIVE" : nullptr);
}
QString fmt_evq_type(std::uint32_t v) {
    const char* n = v == 0x01 ? "SYS_PPU_QUEUE" : v == 0x02 ? "SYS_SPU_QUEUE" : nullptr;
    return hex_with_name(v, 2, n);
}
QString fmt_owner32_init(std::uint32_t v) {
    return v == 0xFFFFFFFFu ? QStringLiteral("0xFFFFFFFF - INITIALIZED") : QStringLiteral("0x%1").arg(v, 8, 16, QChar('0'));
}
QString fmt_waiters(int count, const std::vector<std::uint64_t>& tids) {
    if (count <= 0) return QStringLiteral("(0)");
    QStringList s;
    for (auto t : tids) s.append(QStringLiteral("0x%1").arg(t, 0, 16));
    return QStringLiteral("(%1) - %2").arg(count).arg(s.join(", "));
}

} // namespace

void kernel_explorer_panel::populate_property_table(const QVariantList& attrs) {
    detail_->setSortingEnabled(false);
    detail_->clear();
    detail_->setRowCount(0);
    detail_->setColumnCount(2);
    detail_->setHorizontalHeaderLabels({tr("Attribute"), tr("Value")});
    detail_->setRowCount(attrs.size());
    for (int i = 0; i < attrs.size(); ++i) {
        const auto m = attrs[i].toMap();
        auto* k = new QTableWidgetItem(m.value(QStringLiteral("k")).toString());
        auto* v = new QTableWidgetItem(m.value(QStringLiteral("v")).toString());
        k->setFlags(k->flags() & ~Qt::ItemIsEditable);
        v->setFlags(v->flags() & ~Qt::ItemIsEditable);
        detail_->setItem(i, 0, k);
        detail_->setItem(i, 1, v);
    }
    detail_->resizeColumnsToContents();
}

void kernel_explorer_panel::populate_detail_table(
    std::uint32_t pid, const QString& category, int highlight_row)
{
    detail_->setSortingEnabled(false);
    detail_->clear();
    detail_->setRowCount(0);
    detail_->setColumnCount(0);

    auto set_headers = [this](std::initializer_list<const char*> labels) {
        QStringList hs;
        for (auto* l : labels) hs << QString::fromUtf8(l);
        detail_->setColumnCount(hs.size());
        detail_->setHorizontalHeaderLabels(hs);
    };
    auto set_cell = [this](int r, int c, const QString& v) {
        detail_->setItem(r, c, new sortable_item(v));
    };

    if (category == cat_ppu_threads) {
        const auto& threads = threads_cache_.value(pid);
        set_headers({"Handle", "Priority", "State", "StackAddr", "StackSize", "Name", "Sys Lib.", "Base Priority"});
        detail_->setRowCount(threads.size());
        for (int i = 0; i < threads.size(); ++i) {
            const auto& t = threads[i];
            set_cell(i, 0, hex32(static_cast<std::uint32_t>(t.thread_id)));
            set_cell(i, 1, QStringLiteral("0x%1").arg(t.priority, 8, 16, QChar('0')));
            set_cell(i, 2, QStringLiteral("0x%1 - %2").arg(t.state, 2, 16, QChar('0')).arg(QString::fromUtf8(opentm::tm_core::dbgp::ppu_thread_state_name(t.state))));
            set_cell(i, 3, QStringLiteral("0x%1").arg(t.stack_addr, 8, 16, QChar('0')));
            set_cell(i, 4, QStringLiteral("0x%1").arg(t.stack_size, 8, 16, QChar('0')));
            set_cell(i, 5, QString::fromUtf8(t.name.c_str()));
            set_cell(i, 6, lib_from_sync_name(t.name));
            set_cell(i, 7, QStringLiteral("0x%1").arg(t.base_priority, 8, 16, QChar('0')));
        }
    } else if (category == cat_mutexes) {
        const auto& es = mutexes_cache_.value(pid);
        set_headers({"Handle", "Owner Thread", "Lock Count", "Condition Count","Condition Var. ID", "Protocol", "Recursive","Process Shared", "Adaptive", "Key", "Flags", "Name","Sys Lib.", "Waiting Threads"});
        detail_->setRowCount(es.size());
        for (int i = 0; i < es.size(); ++i) {
            const auto& e = es[i];
            set_cell(i, 0,  hex32(e.handle));
            set_cell(i, 1,  e.owner_thread_id? QStringLiteral("0x%1").arg(e.owner_thread_id, 0, 16): QStringLiteral("0x00"));
            set_cell(i, 2,  QString::number(e.lock_counter));
            set_cell(i, 3,  QString::number(e.cond_ref_counter));
            set_cell(i, 4,  hex32(e.cond_var_id));
            set_cell(i, 5,  fmt_protocol(e.attr_protocol));
            set_cell(i, 6,  fmt_recursive(e.attr_recursive));
            set_cell(i, 7,  fmt_pshared(e.attr_shared));
            set_cell(i, 8,  fmt_adaptive(e.attr_adaptive));
            set_cell(i, 9,  QStringLiteral("0x%1").arg(e.key, 16, 16, QChar('0')));
            set_cell(i, 10, QStringLiteral("0x%1").arg(e.flags, 5, 16, QChar('0')));
            set_cell(i, 11, QString::fromUtf8(e.name.c_str()));
            set_cell(i, 12, lib_from_sync_name(e.name));
            set_cell(i, 13, fmt_waiters(e.wait_thread_count, e.wait_thread_ids));
        }
    } else if (category == cat_lwmutexes) {
        const auto& es = lwmutexes_cache_.value(pid);
        set_headers({"Handle", "Owner Thread", "Lock Count", "Protocol", "Recursive", "Name", "Sys Lib.", "Waiting Threads"});
        detail_->setRowCount(es.size());
        for (int i = 0; i < es.size(); ++i) {
            const auto& e = es[i];
            set_cell(i, 0, hex32(e.handle));
            set_cell(i, 1, fmt_owner32_init(e.owner_thread_id));
            set_cell(i, 2, QString::number(e.lock_counter));
            set_cell(i, 3, fmt_protocol(e.attr_protocol));
            set_cell(i, 4, fmt_recursive(e.attr_recursive));
            set_cell(i, 5, QString::fromUtf8(e.name.c_str()));
            set_cell(i, 6, lib_from_sync_name(e.name));
            set_cell(i, 7, QStringLiteral("(%1)").arg(e.wait_thread_ids.size()));
        }
    } else if (category == cat_conds) {
        const auto& es = conds_cache_.value(pid);
        set_headers({"Handle", "Mutex ID", "Process Shared", "Key", "Flags", "Name", "Sys Lib.", "Waiting Threads"});
        detail_->setRowCount(es.size());
        for (int i = 0; i < es.size(); ++i) {
            const auto& e = es[i];
            set_cell(i, 0, hex32(e.handle));
            set_cell(i, 1, hex32(e.mutex_id));
            set_cell(i, 2, fmt_pshared(e.attr_shared));
            set_cell(i, 3, QStringLiteral("0x%1").arg(e.key, 16, 16, QChar('0')));
            set_cell(i, 4, QStringLiteral("0x%1").arg(e.flags, 5, 16, QChar('0')));
            set_cell(i, 5, QString::fromUtf8(e.name.c_str()));
            set_cell(i, 6, lib_from_sync_name(e.name));
            set_cell(i, 7, fmt_waiters(e.wait_thread_ids.size(), e.wait_thread_ids));
        }
    } else if (category == cat_event_queues) {
        const auto& es = evqs_cache_.value(pid);
        set_headers({"Handle", "Key", "Queue Size", "Protocol", "type", "Name", "Sys Lib.", "Waiting Threads", "Queued Events"});
        detail_->setRowCount(es.size());
        for (int i = 0; i < es.size(); ++i) {
            const auto& e = es[i];
            set_cell(i, 0, hex32(e.handle));
            set_cell(i, 1, QStringLiteral("0x%1").arg(e.key, 0, 16));
            set_cell(i, 2, QStringLiteral("0x%1").arg(e.queue_size, 8, 16, QChar('0')));
            set_cell(i, 3, fmt_protocol(e.attr_protocol));
            set_cell(i, 4, fmt_evq_type(e.type));
            set_cell(i, 5, QString::fromUtf8(e.name.c_str()));
            set_cell(i, 6, lib_from_sync_name(e.name));
            set_cell(i, 7, fmt_waiters(e.wait_thread_ids.size(), e.wait_thread_ids));
            set_cell(i, 8, QString::number(e.queued_count));
        }
    } else if (category == cat_modules) {
        const auto& ms = modules_cache_.value(pid);
        set_headers({"Module ID", "Name", "Mem Size", "Version", "Attribute", "Start", "Stop", "PRX Name", "Number of Segments", "GUID"});
        detail_->setRowCount(ms.size());
        for (int i = 0; i < ms.size(); ++i) {
            const auto& m = ms[i];
            set_cell(i, 0, hex32(m.handle));
            set_cell(i, 1, QString::fromUtf8(m.name.c_str()));
            set_cell(i, 2, QStringLiteral("0x%1 (%2)").arg(m.mem_size, 0, 16).arg(format_bytes(m.mem_size)));
            set_cell(i, 3, QStringLiteral("%1.%2").arg(m.version_major).arg(m.version_minor));
            set_cell(i, 4, hex32(m.attribute));
            set_cell(i, 5, hex32(m.start_entry));
            set_cell(i, 6, hex32(m.stop_entry));
            set_cell(i, 7, QString::fromUtf8(m.path.c_str()));
            set_cell(i, 8, QString::number(m.segments.size()));
            set_cell(i, 9, QStringLiteral("0-0-0-0-0"));
        }
    } else if (category == cat_segments) {
        const auto& ms = modules_cache_.value(pid);
        set_headers({"Module Name", "ID", "Index", "Type", "Start Address", "End Address", "Memory Size", "File Size", "Flags", "Alignment"});
        int total = 0;
        for (const auto& m : ms) total += static_cast<int>(m.segments.size());
        detail_->setRowCount(total);
        int i = 0;
        for (const auto& m : ms) {
            for (const auto& s : m.segments) {
                set_cell(i, 0, QString::fromUtf8(m.name.c_str()));
                set_cell(i, 1, hex32(m.handle));
                set_cell(i, 2, QStringLiteral("0x%1").arg(s.segment_num, 0, 16));
                set_cell(i, 3, seg_kind_name(s.segment_num));
                set_cell(i, 4, QStringLiteral("0x%1").arg(s.base, 0, 16));
                set_cell(i, 5, QStringLiteral("0x%1").arg(s.base + s.mem_size, 0, 16));
                set_cell(i, 6, QStringLiteral("0x%1 (%2)").arg(s.mem_size, 0, 16).arg(format_bytes(s.mem_size)));
                set_cell(i, 7, QStringLiteral("0x%1 (%2)").arg(s.file_size, 0, 16).arg(format_bytes(s.file_size)));
                set_cell(i, 8, fmt_seg_flags(s.attr_flags));
                set_cell(i, 9, QString::number(s.align));
                ++i;
            }
        }
    } else if (category == cat_containers) {
        const auto& es = containers_cache_.value(pid);
        set_headers({"ID", "Parent ID", "Total", "Available"});
        detail_->setRowCount(es.size());
        for (int i = 0; i < es.size(); ++i) {
            const auto& e = es[i];
            set_cell(i, 0, QStringLiteral("Process memory"));
            set_cell(i, 1, e.parent_id == 0xFFFFFFFFu ? QStringLiteral("N/A") : hex32(e.parent_id));
            set_cell(i, 2, format_bytes(e.total));
            set_cell(i, 3, format_bytes(e.available));
        }
    }
    detail_->resizeColumnsToContents();

    if (highlight_row >= 0 && highlight_row < detail_->rowCount()) {
        detail_->selectRow(highlight_row);
        detail_->scrollToItem(detail_->item(highlight_row, 0));
    }
}

bool kernel_explorer_panel::resolve_selection(std::uint32_t& pid_out, QString& category_out, int& row_out) const
{
    pid_out = 0;
    category_out.clear();
    row_out = -1;

    const auto idx = tree_->selectionModel()->currentIndex();
    if (!idx.isValid()) return false;
    auto* item = model_->itemFromIndex(idx);
    if (!item) return false;
    const int kind = item->data(kind_role).toInt();

    if (kind == kind_category) {
        pid_out = static_cast<std::uint32_t>(item->data(pid_role).toUInt());
        category_out = item->data(category_role).toString();
        return true;
    }
    if (kind == kind_entry) {
        auto* cur = item;
        while (cur && cur->data(kind_role).toInt() != kind_category) {
            cur = cur->parent();
        }
        if (!cur) return false;
        pid_out = static_cast<std::uint32_t>(cur->data(pid_role).toUInt());
        category_out = cur->data(category_role).toString();
        auto* top = item;
        while (top && top->parent() && top->parent() != cur) {
            top = top->parent();
        }
        if (top) row_out = top->row();
        return true;
    }
    if (kind == kind_process) {
        pid_out = static_cast<std::uint32_t>(item->data(pid_role).toUInt());
        return true;
    }
    return false;
}

void kernel_explorer_panel::on_tree_selection_changed() {
    const bool has_process = current_process_pid() != 0;
    for (auto* b : { core_dump_btn_, resume_btn_, pause_btn_, terminate_btn_ }) {
        b->setEnabled(has_process);
    }

    const auto idx = tree_->selectionModel()->currentIndex();
    if (idx.isValid()) {
        if (auto* item = model_->itemFromIndex(idx)) {
            if (item->data(kind_role).toInt() == kind_entry) {
                const auto attrs = item->data(payload_role).toList();
                if (!attrs.isEmpty()) {
                    populate_property_table(attrs);
                    copy_value_btn_->setEnabled(detail_->rowCount() > 0);
                    return;
                }
            }
        }
    }

    std::uint32_t pid = 0;
    QString category;
    int row = -1;
    if (!resolve_selection(pid, category, row) || category.isEmpty()) {
        detail_->clear();
        detail_->setRowCount(0);
        detail_->setColumnCount(0);
        copy_value_btn_->setEnabled(false);
        return;
    }
    populate_detail_table(pid, category, -1);
    copy_value_btn_->setEnabled(detail_->rowCount() > 0);
}

} // namespace opentm::tm_ui
