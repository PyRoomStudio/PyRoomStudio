#include "gui/MainWindow.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("PyRoomStudio");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("PyRoomStudio");
    app.setWindowIcon(QIcon(":/logo.svg"));

    // Request OpenGL context with depth buffer
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(2, 1);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    QSurfaceFormat::setDefaultFormat(format);

    prs::MainWindow window;
    window.show();

    return app.exec();
}
