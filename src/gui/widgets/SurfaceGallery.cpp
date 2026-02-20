#include "SurfaceGallery.h"
#include "ColorSwatch.h"

#include <QGridLayout>

namespace prs {

SurfaceGallery::SurfaceGallery(const QString& title,
                                const QVector<AssetsPanel::SurfaceInfo>& surfaces,
                                QWidget* parent)
    : QGroupBox(title, parent)
{
    setCheckable(true);
    setChecked(true);

    auto* grid = new QGridLayout(this);
    grid->setSpacing(4);
    grid->setContentsMargins(4, 4, 4, 4);

    int cols = 2;
    for (int i = 0; i < surfaces.size(); ++i) {
        auto& si = surfaces[i];
        auto* swatch = new ColorSwatch(si.name, si.color, this);

        connect(swatch, &ColorSwatch::clicked, [this, idx = si.index, name = si.name]() {
            emit surfaceClicked(idx, name);
        });

        entries_.push_back({si.index, swatch});
        grid->addWidget(swatch, i / cols, i % cols);
    }
}

void SurfaceGallery::updateColor(int surfaceIndex, const Color3i& color) {
    for (auto& entry : entries_) {
        if (entry.surfaceIndex == surfaceIndex) {
            entry.swatch->setColor(color);
            break;
        }
    }
}

} // namespace prs
