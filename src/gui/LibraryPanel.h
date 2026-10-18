#pragma once

#include "core/Types.h"
#include "core/Material.h"

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <vector>

namespace prs {

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
    void applyFilter();

    QTabWidget* tabWidget_ = nullptr;
    QLineEdit* search_ = nullptr;
    QTreeWidget* materialTree_ = nullptr;
    std::vector<Material> materials_; // indexed by leaf item Qt::UserRole
    QListWidget* soundList_ = nullptr;
    QLabel* soundDirLabel_ = nullptr;
    QString soundDirectory_;
};

} // namespace prs
