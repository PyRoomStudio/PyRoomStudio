#pragma once

#include "SimulationWorker.h"

#include <QMutex>
#include <QObject>
#include <QThread>

#include <deque>
#include <optional>

namespace prs {

class SimulationQueue : public QObject {
    Q_OBJECT

  public:
    enum class JobStatus { Queued, Running, Completed, Failed, Cancelled };

    struct JobInfo {
        int id;
        QString description;
        JobStatus status = JobStatus::Queued;
        int progressPercent = 0;
        QString progressMessage;
        QString outputDir;
        QString errorMessage;
    };

    explicit SimulationQueue(QObject* parent = nullptr);
    ~SimulationQueue() override;

    int enqueue(const SimulationWorker::Params& params, const QString& description);
    void cancelCurrent();
    void cancelAll();
    void removeCompleted(int jobId);

    int queueSize() const;
    bool isRunning() const;
    std::vector<JobInfo> allJobs() const;
    std::optional<JobInfo> currentJob() const;

  signals:
    void jobStarted(int jobId, const QString& description);
    void jobProgress(int jobId, int percent, const QString& message);
    void jobFinished(int jobId, const QString& outputDir);
    void jobError(int jobId, const QString& message);
    void queueChanged();

  private slots:
    void processNext();
    void onWorkerProgress(int percent, const QString& message);
    void onWorkerFinished(const QString& outputDir);
    void onWorkerError(const QString& message);

  private:
    mutable QMutex mutex_;
    int nextJobId_ = 1;
    std::deque<std::pair<int, SimulationWorker::Params>> pending_;
    std::vector<JobInfo> allJobs_;

    QThread* workerThread_ = nullptr;
    SimulationWorker* currentWorker_ = nullptr;
    int currentJobId_ = -1;
};

} // namespace prs
