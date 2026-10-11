#pragma once

#include "core/Types.h"
#include "core/Material.h"

#include <QWidget>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QString>
#include <vector>

namespace prs {

class ColorSwatch;

class MaterialGallery : public QWidget {
    Q_OBJECT

public:
    MaterialGallery(const QString& title, const std::vector<Material>& materials,
                    QWidget* parent = nullptr);

    // Hides non-matching materials; expands while a filter is active.
    void setFilter(const QString& filter);

signals:
    void materialClicked(const Material& material);

private:
    void setCollapsed(bool collapsed);

    QPushButton* header_ = nullptr;
    QWidget* contentWidget_ = nullptr;
    QGridLayout* grid_ = nullptr;
    std::vector<ColorSwatch*> swatches_;
    bool collapsed_ = true;
};

} // namespace prs
