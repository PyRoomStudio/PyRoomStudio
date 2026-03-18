#include "MaterialGallery.h"
#include "ColorSwatch.h"

#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace prs {

static QByteArray serializeMaterial(const Material& mat) {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(mat.name);
    obj["category"] = QString::fromStdString(mat.category);
    obj["thickness"] = QString::fromStdString(mat.thickness);
    obj["scattering"] = static_cast<double>(mat.scattering);
    obj["texture"] = QString::fromStdString(mat.texturePath);

    QJsonObject abs;
    for (int i = 0; i < NUM_FREQ_BANDS; ++i)
        abs[QString::number(FREQ_BANDS[i])] = static_cast<double>(mat.absorption[i]);
    obj["absorption"] = abs;

    QJsonArray c;
    c.append(mat.color[0]);
    c.append(mat.color[1]);
    c.append(mat.color[2]);
    obj["color"] = c;

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

MaterialGallery::MaterialGallery(const QString& title,
                                  const std::vector<Material>& materials,
                                  QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    header_ = new QPushButton(this);
    header_->setText(QString::fromUtf8("\u25B6  ") + title + QString(" (%1)").arg(materials.size()));
    header_->setFlat(true);
    header_->setStyleSheet(
        "QPushButton { text-align: left; font-weight: bold; padding: 5px 8px; "
        "background: #d8d8d8; border: none; border-bottom: 1px solid #bbb; }"
        "QPushButton:hover { background: #c8c8c8; }"
    );
    header_->setCursor(Qt::PointingHandCursor);
    mainLayout->addWidget(header_);

    contentWidget_ = new QWidget(this);
    auto* grid = new QGridLayout(contentWidget_);
    grid->setSpacing(4);
    grid->setContentsMargins(4, 4, 4, 4);

    int cols = 2;
    for (int i = 0; i < static_cast<int>(materials.size()); ++i) {
        auto& mat = materials[i];
        auto* swatch = new ColorSwatch(
            QString::fromStdString(mat.name), mat.color, contentWidget_);

        swatch->setDragData(serializeMaterial(mat));

        connect(swatch, &ColorSwatch::clicked, [this, mat]() {
            emit materialClicked(mat);
        });

        swatches_.push_back(swatch);
        grid->addWidget(swatch, i / cols, i % cols);
    }

    mainLayout->addWidget(contentWidget_);

    setCollapsed(true);

    connect(header_, &QPushButton::clicked, [this]() {
        setCollapsed(!collapsed_);
    });
}

void MaterialGallery::setCollapsed(bool collapsed) {
    collapsed_ = collapsed;
    contentWidget_->setVisible(!collapsed);

    QString title = header_->text();
    if (collapsed) {
        title.replace(0, 1, QString::fromUtf8("\u25B6"));
    } else {
        title.replace(0, 1, QString::fromUtf8("\u25BC"));
    }
    header_->setText(title);
}

} // namespace prs
