#include "RenderOptionsDialog.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>

namespace prs {

RenderOptionsDialog::RenderOptionsDialog(
    const std::vector<ListenerEntry>& listeners, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Render Options");
    setMinimumWidth(360);

    auto* mainLayout = new QVBoxLayout(this);

    // Simulation method selection
    auto* methodGroup = new QGroupBox("Simulation Method");
    auto* methodLayout = new QFormLayout(methodGroup);

    methodCombo_ = new QComboBox;
    methodCombo_->addItem("Ray Tracing (ISM + Monte Carlo)");
    methodCombo_->addItem("DG Wave Solver (2D)");
    methodCombo_->addItem("DG Wave Solver (3D)");
    methodCombo_->setCurrentIndex(0);
    methodLayout->addRow("Method:", methodCombo_);

    // DG-specific parameters (hidden by default)
    dgParamsWidget_ = new QWidget;
    auto* dgLayout = new QFormLayout(dgParamsWidget_);
    dgLayout->setContentsMargins(0, 0, 0, 0);

    polyOrderSpin_ = new QSpinBox;
    polyOrderSpin_->setRange(1, 6);
    polyOrderSpin_->setValue(3);
    polyOrderSpin_->setToolTip("Higher order = more accurate but slower. 3 is recommended.");
    dgLayout->addRow("Polynomial order:", polyOrderSpin_);

    maxFreqSpin_ = new QSpinBox;
    maxFreqSpin_->setRange(100, 10000);
    maxFreqSpin_->setValue(1000);
    maxFreqSpin_->setSingleStep(100);
    maxFreqSpin_->setSuffix(" Hz");
    maxFreqSpin_->setToolTip("Maximum frequency to resolve. Higher = finer mesh = slower.");
    dgLayout->addRow("Max frequency:", maxFreqSpin_);

    dgParamsWidget_->setVisible(false);
    methodLayout->addRow(dgParamsWidget_);

    mainLayout->addWidget(methodGroup);

    connect(methodCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RenderOptionsDialog::onMethodChanged);

    // Listener selection
    mainLayout->addWidget(new QLabel("Select listeners to render:"));

    for (int i = 0; i < static_cast<int>(listeners.size()); ++i) {
        auto* cb = new QCheckBox(QString::fromStdString(listeners[i].name));
        cb->setChecked(listeners[i].selected);
        mainLayout->addWidget(cb);
        checkboxes_.push_back(cb);
    }

    auto* selectRow = new QHBoxLayout;
    auto* selectAllBtn = new QPushButton("Select All");
    auto* deselectAllBtn = new QPushButton("Deselect All");
    selectRow->addWidget(selectAllBtn);
    selectRow->addWidget(deselectAllBtn);
    mainLayout->addLayout(selectRow);

    connect(selectAllBtn, &QPushButton::clicked, [this]() {
        for (auto* cb : checkboxes_) cb->setChecked(true);
    });
    connect(deselectAllBtn, &QPushButton::clicked, [this]() {
        for (auto* cb : checkboxes_) cb->setChecked(false);
    });

    auto* btnRow = new QHBoxLayout;
    auto* renderBtn = new QPushButton("Render");
    auto* cancelBtn = new QPushButton("Cancel");
    renderBtn->setDefault(true);
    btnRow->addStretch();
    btnRow->addWidget(renderBtn);
    btnRow->addWidget(cancelBtn);
    mainLayout->addLayout(btnRow);

    connect(renderBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void RenderOptionsDialog::onMethodChanged(int index) {
    dgParamsWidget_->setVisible(index >= 1);
}

std::vector<int> RenderOptionsDialog::selectedListenerIndices() const {
    std::vector<int> indices;
    for (int i = 0; i < static_cast<int>(checkboxes_.size()); ++i) {
        if (checkboxes_[i]->isChecked())
            indices.push_back(i);
    }
    return indices;
}

SimMethod RenderOptionsDialog::selectedMethod() const {
    switch (methodCombo_->currentIndex()) {
    case 1:  return SimMethod::DG_2D;
    case 2:  return SimMethod::DG_3D;
    default: return SimMethod::RayTracing;
    }
}

int RenderOptionsDialog::dgPolynomialOrder() const {
    return polyOrderSpin_->value();
}

float RenderOptionsDialog::dgMaxFrequency() const {
    return static_cast<float>(maxFreqSpin_->value());
}

} // namespace prs
