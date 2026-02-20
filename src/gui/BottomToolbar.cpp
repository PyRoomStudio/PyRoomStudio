#include "BottomToolbar.h"

#include <QHBoxLayout>

namespace prs {

BottomToolbar::BottomToolbar(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(10);
    layout->addStretch();

    importRoomBtn_  = new QPushButton("Import Room");
    importSoundBtn_ = new QPushButton("Import Sound");
    placePointBtn_  = new QPushButton("Place Point");
    renderBtn_      = new QPushButton("Render");

    layout->addWidget(importRoomBtn_);
    layout->addWidget(importSoundBtn_);
    layout->addWidget(placePointBtn_);
    layout->addWidget(renderBtn_);
    layout->addStretch();

    connect(importRoomBtn_,  &QPushButton::clicked, this, &BottomToolbar::importRoomClicked);
    connect(importSoundBtn_, &QPushButton::clicked, this, &BottomToolbar::importSoundClicked);
    connect(placePointBtn_,  &QPushButton::clicked, this, &BottomToolbar::placePointClicked);
    connect(renderBtn_,      &QPushButton::clicked, this, &BottomToolbar::renderClicked);
}

void BottomToolbar::setPlacePointText(const QString& text) {
    placePointBtn_->setText(text);
}

} // namespace prs
