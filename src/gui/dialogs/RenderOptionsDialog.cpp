#include "RenderOptionsDialog.h"

#include <QLabel>
#include <QHBoxLayout>

namespace prs {

RenderOptionsDialog::RenderOptionsDialog(
    const std::vector<ListenerEntry>& listeners, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Render Options");
    setMinimumWidth(320);

    auto* mainLayout = new QVBoxLayout(this);
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

std::vector<int> RenderOptionsDialog::selectedListenerIndices() const {
    std::vector<int> indices;
    for (int i = 0; i < static_cast<int>(checkboxes_.size()); ++i) {
        if (checkboxes_[i]->isChecked())
            indices.push_back(i);
    }
    return indices;
}

} // namespace prs
