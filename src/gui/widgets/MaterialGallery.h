#pragma once

#include "core/Types.h"
#include "core/Material.h"

#include <QWidget>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>
#include <QString>
#include <vector>

namespace prs {

class ColorSwatch;

class MaterialGallery : public QWidget {
    Q_OBJECT

public:
    MaterialGallery(const QString& title, const std::vector<Material>& materials,
                    QWidget* parent = nullptr);

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
