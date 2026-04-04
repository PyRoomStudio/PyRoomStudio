#include "SimulationQueue.h"

namespace prs {

SimulationQueue::SimulationQueue(QObject* parent)
    : QObject(parent) {}

SimulationQueue::~SimulationQueue() {
    cancelAll();
    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait();
    }
}

int SimulationQueue::enqueue(const SimulationWorker::Params& params, const QString& description) {
    QMutexLocker lock(&mutex_);
    int id = nextJobId_++;
    pending_.push_back({id, params});

    JobInfo info;
    info.id = id;
    info.description = description;
    info.status = JobStatus::Queued;
    allJobs_.push_back(info);

    lock.unlock();
    emit queueChanged();

    QMetaObject::invokeMethod(this, &SimulationQueue::processNext, Qt::QueuedConnection);
    return id;
}

void SimulationQueue::cancelCurrent() {
    QMutexLocker lock(&mutex_);
    if (currentWorker_) {
        currentWorker_->cancel();
    }
}

void SimulationQueue::cancelAll() {
    QMutexLocker lock(&mutex_);
    for (auto& [id, _] : pending_) {
        for (auto& j : allJobs_) {
            if (j.id == id)
                j.status = JobStatus::Cancelled;
        }
    }
    pending_.clear();
    if (currentWorker_) {
        currentWorker_->cancel();
    }
    lock.unlock();
    emit queueChanged();
}

void SimulationQueue::removeCompleted(int jobId) {
    QMutexLocker lock(&mutex_);
    allJobs_.erase(std::remove_if(allJobs_.begin(), allJobs_.end(),
                                  [jobId](const JobInfo& j) {
                                      return j.id == jobId &&
                                             (j.status == JobStatus::Completed || j.status == JobStatus::Failed ||
                                              j.status == JobStatus::Cancelled);
                                  }),
                   allJobs_.end());
    lock.unlock();
    emit queueChanged();
}

int SimulationQueue::queueSize() const {
    QMutexLocker lock(&mutex_);
    return static_cast<int>(pending_.size());
}

bool SimulationQueue::isRunning() const {
    QMutexLocker lock(&mutex_);
    return currentWorker_ != nullptr;
}

std::vector<SimulationQueue::JobInfo> SimulationQueue::allJobs() const {
    QMutexLocker lock(&mutex_);
    return allJobs_;
}

std::optional<SimulationQueue::JobInfo> SimulationQueue::currentJob() const {
    QMutexLocker lock(&mutex_);
    if (currentJobId_ < 0)
        return std::nullopt;
    for (auto& j : allJobs_) {
        if (j.id == currentJobId_)
            return j;
    }
    return std::nullopt;
}

void SimulationQueue::processNext() {
    QMutexLocker lock(&mutex_);
    if (currentWorker_ || pending_.empty())
        return;

    auto [id, params] = pending_.front();
    pending_.pop_front();
    currentJobId_ = id;

    for (auto& j : allJobs_) {
        if (j.id == id) {
            j.status = JobStatus::Running;
            break;
        }
    }

    workerThread_ = new QThread;
    currentWorker_ = new SimulationWorker(params);
    currentWorker_->moveToThread(workerThread_);

    connect(workerThread_, &QThread::started, currentWorker_, &SimulationWorker::process);
    connect(currentWorker_, &SimulationWorker::progressChanged, this, &SimulationQueue::onWorkerProgress);
    connect(currentWorker_, &SimulationWorker::finished, this, &SimulationQueue::onWorkerFinished);
    connect(currentWorker_, &SimulationWorker::error, this, &SimulationQueue::onWorkerError);

    QString desc;
    for (auto& j : allJobs_) {
        if (j.id == id) {
            desc = j.description;
            break;
        }
    }

    lock.unlock();
    emit jobStarted(id, desc);
    emit queueChanged();
    workerThread_->start();
}

void SimulationQueue::onWorkerProgress(int percent, const QString& message) {
    QMutexLocker lock(&mutex_);
    for (auto& j : allJobs_) {
        if (j.id == currentJobId_) {
            j.progressPercent = percent;
            j.progressMessage = message;
            break;
        }
    }
    int id = currentJobId_;
    lock.unlock();
    emit jobProgress(id, percent, message);
    emit queueChanged();
}

void SimulationQueue::onWorkerFinished(const QString& outputDir) {
    QMutexLocker lock(&mutex_);
    int id = currentJobId_;
    for (auto& j : allJobs_) {
        if (j.id == id) {
            j.status = JobStatus::Completed;
            j.outputDir = outputDir;
            j.progressPercent = 100;
            break;
        }
    }

    workerThread_->quit();
    workerThread_->wait();
    currentWorker_->deleteLater();
    workerThread_->deleteLater();
    currentWorker_ = nullptr;
    workerThread_ = nullptr;
    currentJobId_ = -1;

    lock.unlock();
    emit jobFinished(id, outputDir);
    emit queueChanged();

    QMetaObject::invokeMethod(this, &SimulationQueue::processNext, Qt::QueuedConnection);
}

void SimulationQueue::onWorkerError(const QString& message) {
    QMutexLocker lock(&mutex_);
    int id = currentJobId_;
    for (auto& j : allJobs_) {
        if (j.id == id) {
            j.status = (message == "Cancelled") ? JobStatus::Cancelled : JobStatus::Failed;
            j.errorMessage = message;
            break;
        }
    }

    workerThread_->quit();
    workerThread_->wait();
    currentWorker_->deleteLater();
    workerThread_->deleteLater();
    currentWorker_ = nullptr;
    workerThread_ = nullptr;
    currentJobId_ = -1;

    lock.unlock();
    emit jobError(id, message);
    emit queueChanged();

    QMetaObject::invokeMethod(this, &SimulationQueue::processNext, Qt::QueuedConnection);
}

} // namespace prs
