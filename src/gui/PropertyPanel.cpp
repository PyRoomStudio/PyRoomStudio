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

        pointNameEdit_ = new QLineEdit;
        pointNameEdit_->setPlaceholderText("Point name...");
        gl->addWidget(pointNameEdit_);
        connect(pointNameEdit_, &QLineEdit::textChanged, this, &PropertyPanel::pointNameChanged);

        auto* typeRow = new QHBoxLayout;
        sourceBtn_   = new QPushButton("Source");
        listenerBtn_ = new QPushButton("Listener");
        typeRow->addWidget(sourceBtn_);
        typeRow->addWidget(listenerBtn_);
        gl->addLayout(typeRow);

        // Volume (only relevant for sources)
        auto* volRow = new QHBoxLayout;
        volRow->addWidget(new QLabel("Volume:"));
        pointVolumeSlider_ = new QSlider(Qt::Horizontal);
        pointVolumeSlider_->setRange(0, 200);
        pointVolumeSlider_->setValue(100);
        pointVolumeLabel_ = new QLabel("1.0");
        volRow->addWidget(pointVolumeSlider_);
        volRow->addWidget(pointVolumeLabel_);
        gl->addLayout(volRow);

        connect(pointVolumeSlider_, &QSlider::valueChanged, [this](int v) {
            if (updatingSlider_) return;
            float vol = v / 100.0f;
            pointVolumeLabel_->setText(QString::number(vol, 'f', 2));
            emit pointVolumeChanged(vol);
        });

        // Audio file (only relevant for sources)
        pointAudioBtn_ = new QPushButton("Set Audio File");
        gl->addWidget(pointAudioBtn_);
        pointAudioLabel_ = new QLabel("No audio file");
        pointAudioLabel_->setStyleSheet("font-size: 10px; color: #666;");
        pointAudioLabel_->setWordWrap(true);
        gl->addWidget(pointAudioLabel_);
        connect(pointAudioBtn_, &QPushButton::clicked, this, &PropertyPanel::selectPointAudioFile);

        // Listener orientation
        orientationWidget_ = new QWidget;
        auto* orientLayout = new QVBoxLayout(orientationWidget_);
        orientLayout->setContentsMargins(0, 0, 0, 0);
        orientLayout->setSpacing(2);
        orientLayout->addWidget(new QLabel("Facing Direction:"));
        auto* dialRow = new QHBoxLayout;
        orientationDial_ = new QDial;
        orientationDial_->setRange(0, 359);
        orientationDial_->setWrapping(true);
        orientationDial_->setNotchesVisible(true);
        orientationDial_->setNotchTarget(45);
        orientationDial_->setFixedSize(60, 60);
        orientationLabel_ = new QLabel("0\u00B0");
        orientationLabel_->setFixedWidth(36);
        dialRow->addWidget(orientationDial_);
        dialRow->addWidget(orientationLabel_);
        dialRow->addStretch();
        orientLayout->addLayout(dialRow);
        gl->addWidget(orientationWidget_);
        orientationWidget_->setVisible(false);

        connect(orientationDial_, &QDial::valueChanged, [this](int v) {
            if (updatingSlider_) return;
            orientationLabel_->setText(QString("%1\u00B0").arg(v));
            emit pointOrientationYawChanged(static_cast<float>(v));
        });

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

        materialLabel_ = new QLabel("Material: (none)");
        materialLabel_->setStyleSheet("font-size: 10px;");
        gl->addWidget(materialLabel_);

        auto* row = new QHBoxLayout;
        textureBtn_     = new QPushButton("Texture");
        loadTextureBtn_ = new QPushButton("Load Texture...");
        deselectSurfBtn_ = new QPushButton("Deselect");
        row->addWidget(textureBtn_);
        row->addWidget(loadTextureBtn_);
        gl->addLayout(row);
        gl->addWidget(deselectSurfBtn_);

        layout->addWidget(group);

        connect(textureBtn_,      &QPushButton::clicked, this, &PropertyPanel::toggleTexture);
        connect(loadTextureBtn_,  &QPushButton::clicked, this, &PropertyPanel::loadTexture);
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
    pointNameEdit_->setEnabled(enabled);
    sourceBtn_->setEnabled(enabled);
    listenerBtn_->setEnabled(enabled);
    deletePointBtn_->setEnabled(enabled);
    deselectPointBtn_->setEnabled(enabled);
    pointVolumeSlider_->setEnabled(enabled);
    pointAudioBtn_->setEnabled(enabled);
    if (!enabled) {
        sourceBtn_->setText("Source");
        listenerBtn_->setText("Listener");
        pointNameEdit_->clear();
        pointAudioLabel_->setText("No audio file");
        orientationWidget_->setVisible(false);
        updatingSlider_ = true;
        pointVolumeSlider_->setValue(100);
        pointVolumeLabel_->setText("1.00");
        orientationDial_->setValue(0);
        orientationLabel_->setText("0\u00B0");
        updatingSlider_ = false;
    }
}

void PropertyPanel::setSurfaceControlsEnabled(bool enabled) {
    textureBtn_->setEnabled(enabled);
    loadTextureBtn_->setEnabled(enabled);
    deselectSurfBtn_->setEnabled(enabled);
    if (!enabled) materialLabel_->setText("Material: (none)");
}

void PropertyPanel::setMaterialName(const QString& name) {
    materialLabel_->setText("Material: " + (name.isEmpty() ? "(none)" : name));
}

void PropertyPanel::setPointName(const QString& name) {
    pointNameEdit_->blockSignals(true);
    pointNameEdit_->setText(name);
    pointNameEdit_->blockSignals(false);
}

void PropertyPanel::setPointVolume(float volume) {
    updatingSlider_ = true;
    pointVolumeSlider_->setValue(static_cast<int>(volume * 100));
    pointVolumeLabel_->setText(QString::number(volume, 'f', 2));
    updatingSlider_ = false;
}

void PropertyPanel::setPointAudioFile(const QString& filename) {
    pointAudioLabel_->setText(filename.isEmpty() ? "No audio file" : filename);
}

void PropertyPanel::setPointOrientationYaw(float yaw) {
    updatingSlider_ = true;
    orientationDial_->setValue(static_cast<int>(yaw));
    orientationLabel_->setText(QString("%1\u00B0").arg(static_cast<int>(yaw)));
    updatingSlider_ = false;
}

void PropertyPanel::setOrientationControlsVisible(bool visible) {
    orientationWidget_->setVisible(visible);
}

} // namespace prs
