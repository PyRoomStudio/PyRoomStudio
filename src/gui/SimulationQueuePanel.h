#pragma once

#include "acoustics/SimulationQueue.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>

namespace prs {

class SimulationQueuePanel : public QWidget {
    Q_OBJECT

public:
    explicit SimulationQueuePanel(SimulationQueue* queue, QWidget* parent = nullptr);

signals:
    void openResults(const QString& outputDir);

private slots:
    void refreshList();

private:
    SimulationQueue* queue_;
    QListWidget* jobList_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* cancelBtn_ = nullptr;
    QPushButton* cancelAllBtn_ = nullptr;
};

} // namespace prs
