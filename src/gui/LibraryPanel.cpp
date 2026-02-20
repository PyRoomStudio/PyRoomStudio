#include "LibraryPanel.h"
#include "widgets/MaterialGallery.h"

#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>

namespace prs {

LibraryPanel::LibraryPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void LibraryPanel::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(0);

    auto* header = new QLabel("LIBRARY");
    header->setAlignment(Qt::AlignCenter);
    header->setStyleSheet("background: #c8c8c8; font-weight: bold; padding: 4px;");
    layout->addWidget(header);

    tabWidget_ = new QTabWidget;
    tabWidget_->setTabPosition(QTabWidget::North);

    auto* soundTab    = new QWidget;
    auto* materialTab = new QWidget;

    createSoundTab(soundTab);
    createMaterialTab(materialTab);

    tabWidget_->addTab(soundTab, "SOUND");
    tabWidget_->addTab(materialTab, "MATERIAL");

    layout->addWidget(tabWidget_);
}

void LibraryPanel::createSoundTab(QWidget* tab) {
    auto* layout = new QVBoxLayout(tab);
    auto* label  = new QLabel("Voice libraries\n(Future feature)");
    label->setAlignment(Qt::AlignCenter);
    label->setEnabled(false);
    layout->addWidget(label);
    layout->addStretch();
}

void LibraryPanel::createMaterialTab(QWidget* tab) {
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);

    auto* content       = new QWidget;
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(4, 4, 4, 4);
    contentLayout->setSpacing(4);

    for (auto& category : getAcousticMaterials()) {
        auto* gallery = new MaterialGallery(
            QString::fromStdString(category.name), category.materials, content);

        connect(gallery, &MaterialGallery::materialClicked,
            [this](const QString& name, const Color3i& color, float absorption) {
                Color3f glColor = {color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f};
                emit materialSelected(name, glColor, absorption);
            });

        materialGalleries_.push_back(gallery);
        contentLayout->addWidget(gallery);
    }

    contentLayout->addStretch();
    scrollArea->setWidget(content);

    auto* tabLayout = new QVBoxLayout(tab);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->addWidget(scrollArea);
}

} // namespace prs
