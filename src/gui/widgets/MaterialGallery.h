#pragma once

#include "core/Types.h"
#include "core/Material.h"

#include <QWidget>
#include <QGroupBox>
#include <QString>
#include <vector>

namespace prs {

class ColorSwatch;

class MaterialGallery : public QGroupBox {
    Q_OBJECT

public:
    MaterialGallery(const QString& title, const std::vector<Material>& materials,
                    QWidget* parent = nullptr);

signals:
    void materialClicked(const QString& name, const Color3i& color, float absorption);

private:
    std::vector<ColorSwatch*> swatches_;
};

} // namespace prs
