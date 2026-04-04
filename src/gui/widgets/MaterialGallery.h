#pragma once

#include "core/Material.h"
#include "core/Types.h"

#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <vector>

namespace prs {

class ColorSwatch;

class MaterialGallery : public QWidget {
    Q_OBJECT

  public:
    MaterialGallery(const QString& title, const std::vector<Material>& materials, QWidget* parent = nullptr);

  signals:
    void materialClicked(const Material& material);

  private:
    void setCollapsed(bool collapsed);

    QPushButton* header_ = nullptr;
    QWidget* contentWidget_ = nullptr;
    std::vector<ColorSwatch*> swatches_;
    bool collapsed_ = true;
};

} // namespace prs
