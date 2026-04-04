#include "AssetsPanel.h"

#include "core/Types.h"
#include "widgets/SurfaceGallery.h"

#include <QFontMetrics>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>

namespace prs {

AssetsPanel::AssetsPanel(QWidget* parent)
    : QWidget(parent) {
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

    tabWidget_ = new QTabWidget;
    tabWidget_->setTabPosition(QTabWidget::North);

    surfacesTab_ = new QWidget;
    auto* surfLay = new QVBoxLayout(surfacesTab_);
    surfLay->setContentsMargins(4, 4, 4, 4);
    surfLay->setSpacing(4);
    auto* surfScroll = new QScrollArea;
    surfScroll->setWidgetResizable(true);
    surfScroll->setFrameShape(QFrame::NoFrame);
    auto* surfScrollContent = new QWidget;
    contentLayout_ = new QVBoxLayout(surfScrollContent);
    contentLayout_->setContentsMargins(0, 0, 0, 0);
    contentLayout_->setSpacing(4);
    contentLayout_->addStretch();
    surfScroll->setWidget(surfScrollContent);
    surfLay->addWidget(surfScroll);

    auto* sourcesTab = new QWidget;
    auto* srcLay = new QVBoxLayout(sourcesTab);
    srcLay->setContentsMargins(4, 4, 4, 4);
    sourcesList_ = new QListWidget;
    sourcesList_->setAlternatingRowColors(true);
    connect(sourcesList_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        bool ok = false;
        int idx = item->data(Qt::UserRole).toInt(&ok);
        if (ok)
            emit pointListClicked(idx);
    });
    srcLay->addWidget(sourcesList_);
    auto* srcHint = new QLabel("Click to select a source");
    srcHint->setAlignment(Qt::AlignCenter);
    srcHint->setStyleSheet("font-size: 9px;");
    srcLay->addWidget(srcHint);

    auto* listenersTab = new QWidget;
    auto* lstLay = new QVBoxLayout(listenersTab);
    lstLay->setContentsMargins(4, 4, 4, 4);
    listenersList_ = new QListWidget;
    listenersList_->setAlternatingRowColors(true);
    connect(listenersList_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        bool ok = false;
        int idx = item->data(Qt::UserRole).toInt(&ok);
        if (ok)
            emit pointListClicked(idx);
    });
    lstLay->addWidget(listenersList_);
    auto* lstHint = new QLabel("Click to select a listener");
    lstHint->setAlignment(Qt::AlignCenter);
    lstHint->setStyleSheet("font-size: 9px;");
    lstLay->addWidget(lstHint);

    tabWidget_->addTab(surfacesTab_, "SURFACES");
    tabWidget_->addTab(sourcesTab, "SOURCES");
    tabWidget_->addTab(listenersTab, "LISTENERS");

    mainLayout->addWidget(tabWidget_);
}

void AssetsPanel::addStlSurfaces(const QString& stlName, const QVector<SurfaceInfo>& surfaces) {
    clearSurfaces();

    gallery_ = new SurfaceGallery(stlName, surfaces, surfacesTab_);

    connect(gallery_, &SurfaceGallery::surfaceClicked, this, &AssetsPanel::surfaceClicked);

    contentLayout_->insertWidget(0, gallery_);
}

void AssetsPanel::updateSurfaceColor(int surfaceIndex, const Color3i& color) {
    if (gallery_)
        gallery_->updateColor(surfaceIndex, color);
}

void AssetsPanel::updateSurfaceAppearance(int surfaceIndex, const Color3i& color, const QPixmap& textureThumbnail) {
    if (gallery_)
        gallery_->updateSurfaceAppearance(surfaceIndex, color, textureThumbnail);
}

void AssetsPanel::clearSurfaces() {
    if (gallery_) {
        contentLayout_->removeWidget(gallery_);
        gallery_->deleteLater();
        gallery_ = nullptr;
    }
}

void AssetsPanel::updatePointLists(const std::vector<PlacedPoint>& points, int activePointIndex) {
    sourcesList_->clear();
    listenersList_->clear();

    int srcN = 0, lstN = 0;
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        const auto& pt = points[i];
        if (pt.pointType == POINT_TYPE_SOURCE) {
            ++srcN;
            QString label = pt.name.empty() ? QString("Source %1").arg(srcN) : QString::fromStdString(pt.name);
            auto* item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, i);
            sourcesList_->addItem(item);
            if (i == activePointIndex)
                sourcesList_->setCurrentItem(item);
        } else if (pt.pointType == POINT_TYPE_LISTENER) {
            ++lstN;
            QString label = pt.name.empty() ? QString("Listener %1").arg(lstN) : QString::fromStdString(pt.name);
            auto* item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, i);
            listenersList_->addItem(item);
            if (i == activePointIndex)
                listenersList_->setCurrentItem(item);
        }
    }
}

} // namespace prs
