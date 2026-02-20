#include "AssetsPanel.h"
#include "widgets/SurfaceGallery.h"

#include <QLabel>
#include <QScrollArea>

namespace prs {

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
    header->setStyleSheet("background: #c8c8c8; font-weight: bold; padding: 4px;");
    mainLayout->addWidget(header);

    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);

    auto* content = new QWidget;
    contentLayout_ = new QVBoxLayout(content);
    contentLayout_->setContentsMargins(4, 4, 4, 4);
    contentLayout_->setSpacing(4);
    contentLayout_->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

void AssetsPanel::addStlSurfaces(const QString& stlName, const QVector<SurfaceInfo>& surfaces) {
    clearSurfaces();

    gallery_ = new SurfaceGallery(stlName, surfaces,
        static_cast<QWidget*>(contentLayout_->parentWidget()));

    connect(gallery_, &SurfaceGallery::surfaceClicked, this, &AssetsPanel::surfaceClicked);

    contentLayout_->insertWidget(0, gallery_);
}

void AssetsPanel::updateSurfaceColor(int surfaceIndex, const Color3i& color) {
    if (gallery_) gallery_->updateColor(surfaceIndex, color);
}

void AssetsPanel::clearSurfaces() {
    if (gallery_) {
        contentLayout_->removeWidget(gallery_);
        gallery_->deleteLater();
        gallery_ = nullptr;
    }
}

} // namespace prs
