#include "SimulationQueueWindow.h"

#include "acoustics/SimulationQueue.h"
#include "SimulationQueuePanel.h"

#include <QCloseEvent>

namespace prs {

SimulationQueueWindow::SimulationQueueWindow(SimulationQueue* queue, QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle("Simulation Queue");
    resize(520, 320);
    panel_ = new SimulationQueuePanel(queue, this);
    setCentralWidget(panel_);
}

void SimulationQueueWindow::closeEvent(QCloseEvent* event) {
    hide();
    event->ignore();
}

} // namespace prs
