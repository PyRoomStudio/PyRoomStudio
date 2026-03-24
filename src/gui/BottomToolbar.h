#pragma once

#include <QWidget>

class QToolButton;

namespace prs {

class BottomToolbar : public QWidget {
    Q_OBJECT

public:
    explicit BottomToolbar(QWidget* parent = nullptr);

    void setPlacementState(bool placementActive, bool sourceMode);

signals:
    void importRoomClicked();
    void addSourceClicked();
    void addListenerClicked();
    void renderClicked();

private:
    QToolButton* importRoomBtn_   = nullptr;
    QToolButton* addSourceBtn_   = nullptr;
    QToolButton* addListenerBtn_ = nullptr;
    QToolButton* renderBtn_      = nullptr;
};

} // namespace prs
