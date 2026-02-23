#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QTableWidget>

namespace prs {

class PreferencesDialog : public QDialog {
    Q_OBJECT
public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
private:
    void accept() override;
    QLineEdit*  projectDirEdit_ = nullptr;
    QSpinBox*   autoSaveInterval_ = nullptr;
};

class DisplaySettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit DisplaySettingsDialog(QWidget* parent = nullptr);
signals:
    void settingsChanged();
private:
    void accept() override;
    QCheckBox*      gridVisible_ = nullptr;
    QDoubleSpinBox* gridSpacing_ = nullptr;
    QSlider*        transparencyAlpha_ = nullptr;
    QLabel*         alphaLabel_ = nullptr;
    QSpinBox*       markerSize_ = nullptr;
};

class AudioSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit AudioSettingsDialog(QWidget* parent = nullptr);
private:
    void accept() override;
    QComboBox* sampleRate_ = nullptr;
};

class SimulationSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SimulationSettingsDialog(QWidget* parent = nullptr);

    int maxOrder() const;
    int numRays() const;
    float energyAbsorption() const;
    float scattering() const;
    bool airAbsorption() const;

private:
    void accept() override;
    QSpinBox*       maxOrder_ = nullptr;
    QSpinBox*       numRays_ = nullptr;
    QDoubleSpinBox* absorption_ = nullptr;
    QDoubleSpinBox* scattering_ = nullptr;
    QCheckBox*      airAbsorption_ = nullptr;
};

class KeyboardShortcutsDialog : public QDialog {
    Q_OBJECT
public:
    explicit KeyboardShortcutsDialog(QWidget* parent = nullptr);
private:
    QTableWidget* table_ = nullptr;
};

} // namespace prs
