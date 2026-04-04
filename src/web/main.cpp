#include <QApplication>
#include <QFile>
#include <QFont>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QPixmap renderLogo() {
    QSvgRenderer renderer(QStringLiteral(":/logo.svg"));
    if (!renderer.isValid())
        return {};

    QImage image(560, 160, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    renderer.render(&painter);
    return QPixmap::fromImage(image);
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SeicheWeb");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Seiche");

    QWidget window;
    window.setWindowTitle("Seiche WebAssembly Preview");

    auto* layout = new QVBoxLayout(&window);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    QLabel* logo = new QLabel;
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(renderLogo());
    layout->addWidget(logo);

    QLabel* title = new QLabel("Seiche WebAssembly shell");
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* body = new QLabel(
        "This browser target proves the Qt for WebAssembly toolchain, bundle, and resource loading path before "
        "the full editor is ported.");
    body->setWordWrap(true);
    body->setAlignment(Qt::AlignCenter);
    layout->addWidget(body);

    QLabel* resourceState = new QLabel(
        QFile::exists(":/logo.svg") ? "Qt resource bundle: loaded" : "Qt resource bundle: missing");
    resourceState->setAlignment(Qt::AlignCenter);
    layout->addWidget(resourceState);

    window.resize(900, 600);
    window.show();

    return app.exec();
}
