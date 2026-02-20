#pragma once

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QString>

namespace prs {

class PropertyPanel : public QWidget {
    Q_OBJECT

public:
    explicit PropertyPanel(QWidget* parent = nullptr);

    void setScaleValue(float value);
    void setDimensionText(const QString& text);
    void setPointDistanceValue(float value);
    void setPointType(const QString& type);
    void setPointControlsEnabled(bool enabled);
    void setSurfaceControlsEnabled(bool enabled);

signals:
    void scaleChanged(float value);
    void pointDistanceChanged(float value);
    void setPointSource();
    void setPointListener();
    void deletePoint();
    void deselectPoint();
    void toggleTexture();
    void deselectSurface();

private:
    void setupUI();

    QSlider*     scaleSlider_         = nullptr;
    QLabel*      scaleValueLabel_     = nullptr;
    QLabel*      dimensionLabel_      = nullptr;
    QSlider*     pointDistSlider_     = nullptr;
    QLabel*      pointDistValueLabel_ = nullptr;
    QPushButton* sourceBtn_           = nullptr;
    QPushButton* listenerBtn_         = nullptr;
    QPushButton* deletePointBtn_      = nullptr;
    QPushButton* deselectPointBtn_    = nullptr;
    QPushButton* textureBtn_          = nullptr;
    QPushButton* deselectSurfBtn_     = nullptr;

    bool updatingSlider_ = false;
};

} // namespace prs
