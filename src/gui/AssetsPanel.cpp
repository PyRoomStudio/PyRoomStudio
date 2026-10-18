#include "AssetsPanel.h"
#include "widgets/TreeFilter.h"
#include "core/Types.h"

#include <QLabel>
#include <QListWidgetItem>
#include <QTabWidget>
#include <QListWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QLineEdit>
#include <QMenu>
#include <QDropEvent>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>
#include <map>

namespace prs {

namespace {

QString profilePath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profile.json";
}

// A malformed or missing profile reads as an empty object.
QJsonObject readProfile() {
    QFile f(profilePath());
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}

bool isSurface(const QTreeWidgetItem* item) { return item->data(0, Qt::UserRole).isValid(); }

QJsonArray serializeChildren(const QTreeWidgetItem* item) {
    QJsonArray out;
    for (int i = 0; i < item->childCount(); ++i) {
        const QTreeWidgetItem* c = item->child(i);
        if (isSurface(c))
            out.append(c->data(0, Qt::UserRole).toInt());
        else
            out.append(QJsonObject{{"name", c->text(0)}, {"children", serializeChildren(c)}});
    }
    return out;
}

// Reports internal drag-and-drop moves, which QTreeWidget has no signal for.
class SurfaceTree : public QTreeWidget {
public:
    std::function<void()> onMoved;
protected:
    void dropEvent(QDropEvent* event) override {
        QTreeWidget::dropEvent(event);
        if (event->isAccepted() && onMoved) onMoved();
    }
};

} // namespace

QJsonArray loadSurfaceGroups(const QString& modelPath) {
    return readProfile()["surfaceGroups"].toObject()[modelPath].toArray();
}

bool saveSurfaceGroups(const QString& modelPath, const QJsonArray& tree) {
    QJsonObject profile = readProfile();
    QJsonObject groups = profile["surfaceGroups"].toObject();
    groups[modelPath] = tree;
    profile["surfaceGroups"] = groups;

    QDir().mkpath(QFileInfo(profilePath()).path());
    QSaveFile out(profilePath());
    if (!out.open(QIODevice::WriteOnly)) return false;
    out.write(QJsonDocument(profile).toJson());
    return out.commit();
}

AssetsPanel::AssetsPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void AssetsPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(0);

    auto* header = new QLabel("ASSETS");
    header->setAlignment(Qt::AlignCenter);
    header->setFixedHeight(QFontMetrics(header->font()).height() + 8);
    header->setStyleSheet("background: #e0e0e0; font-weight: bold; padding: 0 4px;");
    mainLayout->addWidget(header);

    search_ = new QLineEdit;
    search_->setPlaceholderText("Search assets...");
    search_->setClearButtonEnabled(true);
    connect(search_, &QLineEdit::textChanged, this, &AssetsPanel::applyFilter);
    mainLayout->addWidget(search_);

    tabWidget_ = new QTabWidget;
    tabWidget_->setTabPosition(QTabWidget::North);

    auto* surfacesTab = new QWidget;
    auto* surfLay     = new QVBoxLayout(surfacesTab);
    surfLay->setContentsMargins(4, 4, 4, 4);
    auto* tree = new SurfaceTree;
    tree->onMoved = [this]() { emit surfaceTreeChanged(); };
    surfaceTree_ = tree;
    surfaceTree_->setHeaderHidden(true);
    surfaceTree_->setColumnCount(1);
    surfaceTree_->setDragDropMode(QAbstractItemView::InternalMove);
    surfaceTree_->setDefaultDropAction(Qt::MoveAction);
    surfaceTree_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    surfaceTree_->invisibleRootItem()->setFlags(Qt::ItemIsEnabled); // nothing dropped beside the model item
    surfaceTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(surfaceTree_, &QWidget::customContextMenuRequested, this, &AssetsPanel::showSurfaceMenu);
    connect(surfaceTree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item) {
        if (isSurface(item))
            emit surfaceClicked(item->data(0, Qt::UserRole).toInt(), item->text(0));
    });
    // Programmatic edits block the tree's signals, so this only sees user renames.
    connect(surfaceTree_, &QTreeWidget::itemChanged, this, [this]() { emit surfaceTreeChanged(); });
    surfLay->addWidget(surfaceTree_);

    auto* sourcesTab = new QWidget;
    auto* srcLay     = new QVBoxLayout(sourcesTab);
    srcLay->setContentsMargins(4, 4, 4, 4);
    sourcesList_ = new QListWidget;
    sourcesList_->setAlternatingRowColors(true);
    connect(sourcesList_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        bool ok = false;
        int idx = item->data(Qt::UserRole).toInt(&ok);
        if (ok) emit pointListClicked(idx);
    });
    srcLay->addWidget(sourcesList_);

    auto* listenersTab = new QWidget;
    auto* lstLay       = new QVBoxLayout(listenersTab);
    lstLay->setContentsMargins(4, 4, 4, 4);
    listenersList_ = new QListWidget;
    listenersList_->setAlternatingRowColors(true);
    connect(listenersList_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        bool ok = false;
        int idx = item->data(Qt::UserRole).toInt(&ok);
        if (ok) emit pointListClicked(idx);
    });
    lstLay->addWidget(listenersList_);

    tabWidget_->addTab(surfacesTab, "SURFACES");
    tabWidget_->addTab(sourcesTab, "SOURCES");
    tabWidget_->addTab(listenersTab, "LISTENERS");

    mainLayout->addWidget(tabWidget_);
}

void AssetsPanel::addStlSurfaces(const QString& stlName, const QVector<SurfaceInfo>& surfaces) {
    clearSurfaces();
    QSignalBlocker block(surfaceTree_);
    modelItem_ = new QTreeWidgetItem(surfaceTree_, {stlName});
    modelItem_->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    for (const auto& si : surfaces) {
        auto* leaf = new QTreeWidgetItem(modelItem_, {si.name});
        leaf->setData(0, Qt::UserRole, si.index);
        leaf->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
    }
    modelItem_->setExpanded(true);
    applyFilter();
}

QTreeWidgetItem* AssetsPanel::addGroup(QTreeWidgetItem* parent, const QString& name) {
    auto* group = new QTreeWidgetItem(parent, {name});
    group->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable |
                    Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    QFont bold = group->font(0);
    bold.setBold(true);
    group->setFont(0, bold);
    return group;
}

QJsonArray AssetsPanel::surfaceTree() const {
    return modelItem_ ? serializeChildren(modelItem_) : QJsonArray();
}

void AssetsPanel::restoreSurfaceTree(const QJsonArray& tree) {
    if (!modelItem_) return;
    QSignalBlocker block(surfaceTree_);

    // Detach every surface leaf and drop the old groups; groups are rebuilt from `tree`.
    std::map<int, QTreeWidgetItem*> leaves;
    std::function<void(QTreeWidgetItem*)> detach = [&](QTreeWidgetItem* item) {
        while (item->childCount() > 0) {
            QTreeWidgetItem* child = item->takeChild(0);
            if (isSurface(child)) {
                leaves[child->data(0, Qt::UserRole).toInt()] = child;
            } else {
                detach(child);
                delete child;
            }
        }
    };
    detach(modelItem_);

    // Unknown or repeated indices find no leaf and are skipped.
    std::function<void(QTreeWidgetItem*, const QJsonArray&)> build =
        [&](QTreeWidgetItem* parent, const QJsonArray& entries) {
            for (const QJsonValue& v : entries) {
                if (v.isDouble()) {
                    auto it = leaves.find(v.toInt(-1));
                    if (it == leaves.end()) continue;
                    parent->addChild(it->second);
                    leaves.erase(it);
                } else if (v.isObject() && v["name"].isString()) {
                    build(addGroup(parent, v["name"].toString()), v["children"].toArray());
                }
            }
        };
    build(modelItem_, tree);
    for (auto& [index, leaf] : leaves)
        modelItem_->addChild(leaf);

    surfaceTree_->expandAll();
    applyFilter();
}

void AssetsPanel::showSurfaceMenu(const QPoint& pos) {
    if (!modelItem_) return;
    QTreeWidgetItem* item = surfaceTree_->itemAt(pos);
    const bool onGroup = item && item != modelItem_ && !isSurface(item);
    // New groups go inside a clicked group, beside a clicked surface, else under the model.
    QTreeWidgetItem* parent = onGroup ? item
                            : (item && item != modelItem_) ? item->parent() : modelItem_;

    QMenu menu(this);
    menu.addAction("New Group", this, [&]() {
        QTreeWidgetItem* group;
        {
            QSignalBlocker block(surfaceTree_);
            group = addGroup(parent, "New Group");
        }
        parent->setExpanded(true);
        emit surfaceTreeChanged();
        surfaceTree_->editItem(group);
    });
    if (onGroup) {
        menu.addAction("Rename Group", this, [&]() { surfaceTree_->editItem(item); });
        menu.addAction("Delete Group", this, [&]() {
            // Children move up into the group's place.
            QTreeWidgetItem* up = item->parent();
            up->insertChildren(up->indexOfChild(item), item->takeChildren());
            delete item;
            emit surfaceTreeChanged();
        });
    }
    menu.exec(surfaceTree_->viewport()->mapToGlobal(pos));
}

void AssetsPanel::applyFilter() {
    const QString text = search_->text();
    for (auto* list : {sourcesList_, listenersList_}) {
        for (int i = 0; i < list->count(); ++i) {
            auto* item = list->item(i);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
    }
    if (modelItem_) filterTreeItem(modelItem_, text);
}

void AssetsPanel::clearSurfaces() {
    surfaceTree_->clear();
    modelItem_ = nullptr;
}

void AssetsPanel::updatePointLists(const std::vector<PlacedPoint>& points, int activePointIndex) {
    sourcesList_->clear();
    listenersList_->clear();

    int srcN = 0, lstN = 0;
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        const auto& pt = points[i];
        if (pt.pointType == POINT_TYPE_SOURCE) {
            ++srcN;
            QString label = pt.name.empty()
                ? QString("Source %1").arg(srcN)
                : QString::fromStdString(pt.name);
            auto* item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, i);
            sourcesList_->addItem(item);
            if (i == activePointIndex) sourcesList_->setCurrentItem(item);
        } else if (pt.pointType == POINT_TYPE_LISTENER) {
            ++lstN;
            QString label = pt.name.empty()
                ? QString("Listener %1").arg(lstN)
                : QString::fromStdString(pt.name);
            auto* item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, i);
            listenersList_->addItem(item);
            if (i == activePointIndex) listenersList_->setCurrentItem(item);
        }
    }
    applyFilter();
}

} // namespace prs
