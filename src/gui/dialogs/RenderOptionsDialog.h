#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <vector>
#include <string>

namespace prs {

class RenderOptionsDialog : public QDialog {
    Q_OBJECT

public:
    struct ListenerEntry {
        std::string name;
        bool selected = true;
    };

    explicit RenderOptionsDialog(const std::vector<ListenerEntry>& listeners,
                                 QWidget* parent = nullptr);

    std::vector<int> selectedListenerIndices() const;

private:
    std::vector<QCheckBox*> checkboxes_;
};

} // namespace prs
