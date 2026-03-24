#pragma once

#include <QMainWindow>

namespace prs {

class SimulationQueue;
class SimulationQueuePanel;

class SimulationQueueWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit SimulationQueueWindow(SimulationQueue* queue, QWidget* parent = nullptr);

    SimulationQueuePanel* panel() const { return panel_; }

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    SimulationQueuePanel* panel_ = nullptr;
};

} // namespace prs
