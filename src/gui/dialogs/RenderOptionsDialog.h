#pragma once

#include "acoustics/SimulationWorker.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <string>
#include <vector>

namespace prs {

class RenderOptionsDialog : public QDialog {
    Q_OBJECT

  public:
    struct ListenerEntry {
        std::string name;
        bool selected = true;
    };

    explicit RenderOptionsDialog(const std::vector<ListenerEntry>& listeners, QWidget* parent = nullptr);

    std::vector<int> selectedListenerIndices() const;
    SimMethod selectedMethod() const;
    int dgPolynomialOrder() const;
    float dgMaxFrequency() const;

  private slots:
    void onMethodChanged(int index);

  private:
    std::vector<QCheckBox*> checkboxes_;
    QComboBox* methodCombo_ = nullptr;
    QSpinBox* polyOrderSpin_ = nullptr;
    QSpinBox* maxFreqSpin_ = nullptr;
    QWidget* dgParamsWidget_ = nullptr;
};

} // namespace prs
