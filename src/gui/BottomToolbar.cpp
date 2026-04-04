#include "BottomToolbar.h"

#include "IconUtils.h"

#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QToolButton>

namespace prs {

namespace {

QToolButton* makeToolbarButton(const QString& text, const QString& iconQrcPath) {
    auto* b = new QToolButton;
    b->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    b->setIcon(iconFromSvgResource(iconQrcPath));
    b->setIconSize(QSize(32, 32));
    b->setText(text);
    b->setAutoRaise(true);
    b->setFocusPolicy(Qt::NoFocus);
    return b;
}

} // namespace

BottomToolbar::BottomToolbar(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(10);
    layout->addStretch();

    importRoomBtn_ = makeToolbarButton("Import Room", ":/toolbar/import_room.svg");
    addSourceBtn_ = makeToolbarButton("Add Source", ":/toolbar/place_sound.svg");
    addListenerBtn_ = makeToolbarButton("Add Listener", ":/toolbar/place_listener.svg");
    renderBtn_ = makeToolbarButton("Render", ":/toolbar/render.svg");

    addSourceBtn_->setCheckable(true);
    addListenerBtn_->setCheckable(true);

    layout->addWidget(importRoomBtn_);
    layout->addWidget(addSourceBtn_);
    layout->addWidget(addListenerBtn_);
    layout->addWidget(renderBtn_);
    layout->addStretch();

    connect(importRoomBtn_, &QToolButton::clicked, this, &BottomToolbar::importRoomClicked);
    connect(addSourceBtn_, &QToolButton::clicked, this, &BottomToolbar::addSourceClicked);
    connect(addListenerBtn_, &QToolButton::clicked, this, &BottomToolbar::addListenerClicked);
    connect(renderBtn_, &QToolButton::clicked, this, &BottomToolbar::renderClicked);
}

void BottomToolbar::setPlacementState(bool placementActive, bool sourceMode) {
    QSignalBlocker b1(addSourceBtn_);
    QSignalBlocker b2(addListenerBtn_);
    addSourceBtn_->setChecked(placementActive && sourceMode);
    addListenerBtn_->setChecked(placementActive && !sourceMode);
}

} // namespace prs
