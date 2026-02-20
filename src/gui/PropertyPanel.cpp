#include "PropertyPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>

namespace prs {

PropertyPanel::PropertyPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PropertyPanel::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto* header = new QLabel("PROPERTY");
    header->setAlignment(Qt::AlignCenter);
    header->setStyleSheet("background: #c8c8c8; font-weight: bold; padding: 4px;");
    layout->addWidget(header);

    // Room Scale
    {
        auto* group = new QGroupBox("Room Scale");
        auto* gl    = new QVBoxLayout(group);
        auto* row   = new QHBoxLayout;
        scaleSlider_ = new QSlider(Qt::Horizontal);
        scaleSlider_->setRange(1, 100);
        scaleSlider_->setValue(10);
        scaleValueLabel_ = new QLabel("1.0x");
        row->addWidget(scaleSlider_);
        row->addWidget(scaleValueLabel_);
        gl->addLayout(row);

        dimensionLabel_ = new QLabel("Size: -- m");
        dimensionLabel_->setStyleSheet("font-size: 10px;");
        gl->addWidget(dimensionLabel_);
        layout->addWidget(group);

        connect(scaleSlider_, &QSlider::valueChanged, [this](int v) {
            if (updatingSlider_) return;
            float value = v / 10.0f;
            scaleValueLabel_->setText(QString("%1x").arg(value, 0, 'f', 1));
            emit scaleChanged(value);
        });
    }

    // Point Distance
    {
        auto* group = new QGroupBox("Point Distance");
        auto* gl    = new QVBoxLayout(group);
        auto* row   = new QHBoxLayout;
        pointDistSlider_ = new QSlider(Qt::Horizontal);
        pointDistSlider_->setRange(-50, 50);
        pointDistSlider_->setValue(0);
        pointDistValueLabel_ = new QLabel("0.0m");
        row->addWidget(pointDistSlider_);
        row->addWidget(pointDistValueLabel_);
        gl->addLayout(row);
        layout->addWidget(group);

        connect(pointDistSlider_, &QSlider::valueChanged, [this](int v) {
            if (updatingSlider_) return;
            float value = v / 10.0f;
            pointDistValueLabel_->setText(QString("%1m").arg(value, 0, 'f', 1));
            emit pointDistanceChanged(value);
        });
    }

    // Point Controls
    {
        auto* group = new QGroupBox("Selected Point");
        auto* gl    = new QVBoxLayout(group);

        auto* typeRow = new QHBoxLayout;
        sourceBtn_   = new QPushButton("Source");
        listenerBtn_ = new QPushButton("Listener");
        typeRow->addWidget(sourceBtn_);
        typeRow->addWidget(listenerBtn_);
        gl->addLayout(typeRow);

        auto* actionRow = new QHBoxLayout;
        deletePointBtn_  = new QPushButton("Delete");
        deselectPointBtn_ = new QPushButton("Deselect");
        actionRow->addWidget(deletePointBtn_);
        actionRow->addWidget(deselectPointBtn_);
        gl->addLayout(actionRow);

        layout->addWidget(group);

        connect(sourceBtn_,        &QPushButton::clicked, this, &PropertyPanel::setPointSource);
        connect(listenerBtn_,      &QPushButton::clicked, this, &PropertyPanel::setPointListener);
        connect(deletePointBtn_,   &QPushButton::clicked, this, &PropertyPanel::deletePoint);
        connect(deselectPointBtn_, &QPushButton::clicked, this, &PropertyPanel::deselectPoint);

        setPointControlsEnabled(false);
    }

    // Surface Controls
    {
        auto* group = new QGroupBox("Selected Surface");
        auto* gl    = new QVBoxLayout(group);

        auto* row = new QHBoxLayout;
        textureBtn_     = new QPushButton("Texture");
        deselectSurfBtn_ = new QPushButton("Deselect");
        row->addWidget(textureBtn_);
        row->addWidget(deselectSurfBtn_);
        gl->addLayout(row);

        layout->addWidget(group);

        connect(textureBtn_,      &QPushButton::clicked, this, &PropertyPanel::toggleTexture);
        connect(deselectSurfBtn_, &QPushButton::clicked, this, &PropertyPanel::deselectSurface);

        setSurfaceControlsEnabled(false);
    }

    layout->addStretch();
}

void PropertyPanel::setScaleValue(float value) {
    updatingSlider_ = true;
    scaleSlider_->setValue(static_cast<int>(value * 10));
    scaleValueLabel_->setText(QString("%1x").arg(value, 0, 'f', 1));
    updatingSlider_ = false;
}

void PropertyPanel::setDimensionText(const QString& text) {
    dimensionLabel_->setText("Size: " + text);
}

void PropertyPanel::setPointDistanceValue(float value) {
    updatingSlider_ = true;
    pointDistSlider_->setValue(static_cast<int>(value * 10));
    pointDistValueLabel_->setText(QString("%1m").arg(value, 0, 'f', 1));
    updatingSlider_ = false;
}

void PropertyPanel::setPointType(const QString& type) {
    if (type == "source") {
        sourceBtn_->setText(QString::fromUtf8("\u25CF Source"));
        listenerBtn_->setText("Listener");
    } else if (type == "listener") {
        sourceBtn_->setText("Source");
        listenerBtn_->setText(QString::fromUtf8("\u25CF Listener"));
    } else {
        sourceBtn_->setText("Source");
        listenerBtn_->setText("Listener");
    }
}

void PropertyPanel::setPointControlsEnabled(bool enabled) {
    sourceBtn_->setEnabled(enabled);
    listenerBtn_->setEnabled(enabled);
    deletePointBtn_->setEnabled(enabled);
    deselectPointBtn_->setEnabled(enabled);
    if (!enabled) {
        sourceBtn_->setText("Source");
        listenerBtn_->setText("Listener");
    }
}

void PropertyPanel::setSurfaceControlsEnabled(bool enabled) {
    textureBtn_->setEnabled(enabled);
    deselectSurfBtn_->setEnabled(enabled);
}

} // namespace prs
