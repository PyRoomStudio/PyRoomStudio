#include "SimulationQueuePanel.h"

#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>

namespace prs {

SimulationQueuePanel::SimulationQueuePanel(SimulationQueue* queue, QWidget* parent)
    : QWidget(parent), queue_(queue)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto* headerLayout = new QHBoxLayout;
    headerLayout->addWidget(new QLabel("<b>Simulation Queue</b>"));
    headerLayout->addStretch();

    cancelBtn_ = new QPushButton("Cancel Current");
    cancelBtn_->setFixedWidth(110);
    cancelBtn_->setEnabled(false);
    headerLayout->addWidget(cancelBtn_);

    cancelAllBtn_ = new QPushButton("Cancel All");
    cancelAllBtn_->setFixedWidth(80);
    cancelAllBtn_->setEnabled(false);
    headerLayout->addWidget(cancelAllBtn_);

    layout->addLayout(headerLayout);

    progressBar_ = new QProgressBar;
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(true);
    progressBar_->setFixedHeight(18);
    layout->addWidget(progressBar_);

    statusLabel_ = new QLabel("No simulations running");
    statusLabel_->setStyleSheet("font-size: 11px;");
    layout->addWidget(statusLabel_);

    jobList_ = new QListWidget;
    jobList_->setAlternatingRowColors(true);
    layout->addWidget(jobList_);

    connect(cancelBtn_, &QPushButton::clicked, queue_, &SimulationQueue::cancelCurrent);
    connect(cancelAllBtn_, &QPushButton::clicked, queue_, &SimulationQueue::cancelAll);
    connect(queue_, &SimulationQueue::queueChanged, this, &SimulationQueuePanel::refreshList);

    connect(jobList_, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
        QString dir = item->data(Qt::UserRole).toString();
        if (!dir.isEmpty())
            emit openResults(dir);
    });

    refreshList();
}

void SimulationQueuePanel::refreshList() {
    auto jobs = queue_->allJobs();
    jobList_->clear();

    bool anyRunning = false;
    bool anyQueued = false;

    for (auto& j : jobs) {
        QString statusStr;
        switch (j.status) {
        case SimulationQueue::JobStatus::Queued:
            statusStr = "[Queued]";
            anyQueued = true;
            break;
        case SimulationQueue::JobStatus::Running:
            statusStr = QString("[Running %1%]").arg(j.progressPercent);
            anyRunning = true;
            break;
        case SimulationQueue::JobStatus::Completed:
            statusStr = "[Done]";
            break;
        case SimulationQueue::JobStatus::Failed:
            statusStr = "[Failed]";
            break;
        case SimulationQueue::JobStatus::Cancelled:
            statusStr = "[Cancelled]";
            break;
        }

        auto* item = new QListWidgetItem(QString("%1 %2").arg(statusStr, j.description));
        if (!j.outputDir.isEmpty())
            item->setData(Qt::UserRole, j.outputDir);
        if (j.status == SimulationQueue::JobStatus::Completed)
            item->setToolTip("Double-click to view results and metrics");
        jobList_->addItem(item);
    }

    cancelBtn_->setEnabled(anyRunning);
    cancelAllBtn_->setEnabled(anyRunning || anyQueued);

    if (anyRunning) {
        auto current = queue_->currentJob();
        if (current) {
            progressBar_->setValue(current->progressPercent);
            statusLabel_->setText(current->progressMessage);
        }
    } else {
        progressBar_->setValue(0);
        statusLabel_->setText(jobs.empty() ? "No simulations running"
                                           : "Queue idle");
    }
}

} // namespace prs
