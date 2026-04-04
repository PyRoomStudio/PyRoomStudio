#pragma once

#include "core/Material.h"
#include "core/Types.h"

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <vector>

namespace prs {

class MaterialGallery;

class LibraryPanel : public QWidget {
    Q_OBJECT

  public:
    explicit LibraryPanel(QWidget* parent = nullptr);

  signals:
    void materialSelected(const Material& material);
    void soundFileSelected(const QString& filepath);

  private:
    void setupUI();
    void createSoundTab(QWidget* tab);
    void createMaterialTab(QWidget* tab);
    void scanSoundDirectory(const QString& dir);

    QTabWidget* tabWidget_ = nullptr;
    std::vector<MaterialGallery*> materialGalleries_;
    QListWidget* soundList_ = nullptr;
    QLabel* soundDirLabel_ = nullptr;
    QString soundDirectory_;
};

} // namespace prs
