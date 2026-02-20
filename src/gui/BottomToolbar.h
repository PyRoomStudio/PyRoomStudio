#pragma once

#include <QWidget>
#include <QPushButton>

namespace prs {

class BottomToolbar : public QWidget {
    Q_OBJECT

public:
    explicit BottomToolbar(QWidget* parent = nullptr);

    void setPlacePointText(const QString& text);

signals:
    void importRoomClicked();
    void importSoundClicked();
    void placePointClicked();
    void renderClicked();

private:
    QPushButton* importRoomBtn_  = nullptr;
    QPushButton* importSoundBtn_ = nullptr;
    QPushButton* placePointBtn_  = nullptr;
    QPushButton* renderBtn_      = nullptr;
};

} // namespace prs
