#include "LibraryPanel.h"
#include "widgets/MaterialGallery.h"
#include "core/MaterialLoader.h"

#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QCoreApplication>
#include <QFontMetrics>

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
    header->setFixedHeight(QFontMetrics(header->font()).height() + 8);
    header->setStyleSheet("background: #e0e0e0; font-weight: bold; padding: 0 4px;");
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
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    soundDirLabel_ = new QLabel("No folder selected");
    soundDirLabel_->setStyleSheet("font-size: 10px;");
    soundDirLabel_->setWordWrap(true);
    layout->addWidget(soundDirLabel_);

    auto* browseBtn = new QPushButton("Browse Folder...");
    connect(browseBtn, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Sound Folder");
        if (!dir.isEmpty()) {
            scanSoundDirectory(dir);
            QSettings s("PyRoomStudio", "PyRoomStudio");
            s.setValue("soundDirectory", dir);
        }
    });
    layout->addWidget(browseBtn);

    soundList_ = new QListWidget;
    soundList_->setAlternatingRowColors(true);
    connect(soundList_, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
        QString filepath = item->data(Qt::UserRole).toString();
        emit soundFileSelected(filepath);
    });
    layout->addWidget(soundList_);

    auto* hintLabel = new QLabel("Double-click to select a sound file");
    hintLabel->setStyleSheet("font-size: 9px;");
    hintLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(hintLabel);

    QSettings s("PyRoomStudio", "PyRoomStudio");
    QString savedDir = s.value("soundDirectory", "sounds/sources").toString();
    if (QDir(savedDir).exists())
        scanSoundDirectory(savedDir);
}

void LibraryPanel::scanSoundDirectory(const QString& dir) {
    soundDirectory_ = dir;
    soundDirLabel_->setText(QDir(dir).dirName());
    soundDirLabel_->setToolTip(dir);

    soundList_->clear();

    QDir directory(dir);
    QStringList filters = {"*.wav", "*.mp3", "*.flac", "*.ogg"};
    QFileInfoList files = directory.entryInfoList(filters, QDir::Files, QDir::Name);

    for (auto& fi : files) {
        auto* item = new QListWidgetItem(fi.fileName());
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        item->setToolTip(fi.absoluteFilePath());
        soundList_->addItem(item);
    }
}

void LibraryPanel::createMaterialTab(QWidget* tab) {
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);

    auto* content       = new QWidget;
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(4, 4, 4, 4);
    contentLayout->setSpacing(4);

    // Load materials from the materials/ directory next to the executable
    QString materialsDir = QCoreApplication::applicationDirPath() + "/materials";
    if (!QDir(materialsDir).exists()) {
        // Fall back to source tree path for development
        materialsDir = "materials";
    }

    auto categories = MaterialLoader::loadFromDirectory(materialsDir);

    for (auto& category : categories) {
        auto* gallery = new MaterialGallery(
            QString::fromStdString(category.name), category.materials, content);

        connect(gallery, &MaterialGallery::materialClicked,
            [this](const Material& mat) {
                emit materialSelected(mat);
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
