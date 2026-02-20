#include "MaterialGallery.h"
#include "ColorSwatch.h"

#include <QGridLayout>

namespace prs {

MaterialGallery::MaterialGallery(const QString& title,
                                  const std::vector<Material>& materials,
                                  QWidget* parent)
    : QGroupBox(title, parent)
{
    setCheckable(true);
    setChecked(false);

    auto* grid = new QGridLayout(this);
    grid->setSpacing(4);
    grid->setContentsMargins(4, 4, 4, 4);

    int cols = 2;
    for (int i = 0; i < static_cast<int>(materials.size()); ++i) {
        auto& mat = materials[i];
        auto* swatch = new ColorSwatch(
            QString::fromStdString(mat.name), mat.color, this);

        connect(swatch, &ColorSwatch::clicked, [this, name = mat.name, color = mat.color, abs = mat.energyAbsorption]() {
            emit materialClicked(QString::fromStdString(name), color, abs);
        });

        swatches_.push_back(swatch);
        grid->addWidget(swatch, i / cols, i % cols);
    }
}

} // namespace prs
