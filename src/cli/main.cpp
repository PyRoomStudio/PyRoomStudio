#include "acoustics/RenderOptions.h"
#include "acoustics/RenderPipeline.h"
#include "acoustics/SimulationWorker.h"
#include "core/ProjectFile.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>

#include <vector>

namespace {

bool parseOnOff(const QString& value, bool* out) {
    const QString normalized = value.trimmed().toLower();
    if (normalized == "on" || normalized == "true" || normalized == "1") {
        *out = true;
        return true;
    }
    if (normalized == "off" || normalized == "false" || normalized == "0") {
        *out = false;
        return true;
    }
    return false;
}

bool parseMethod(const QString& value, prs::RenderMethod* out) {
    const QString normalized = value.trimmed().toLower();
    if (normalized == "ray") {
        *out = prs::RenderMethod::RayTracing;
        return true;
    }
    if (normalized == "dg2d") {
        *out = prs::RenderMethod::DG_2D;
        return true;
    }
    if (normalized == "dg3d") {
        *out = prs::RenderMethod::DG_3D;
        return true;
    }
    return false;
}

void printError(const QString& message) {
    QTextStream err(stderr);
    err << message << '\n';
    err.flush();
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("SeicheRender");
    QCoreApplication::setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Headless render entry point for Seiche");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption projectOpt(QStringList() << "p" << "project", "Project file to render.", "file");
    QCommandLineOption outputOpt(QStringList() << "o" << "output", "Output directory.", "dir");
    QCommandLineOption methodOpt("method", "Render method: ray, dg2d, or dg3d.", "method");
    QCommandLineOption maxOrderOpt("max-order", "Maximum ray tracing order.", "order");
    QCommandLineOption nRaysOpt("n-rays", "Number of rays.", "count");
    QCommandLineOption scatteringOpt("scattering", "Scattering coefficient.", "value");
    QCommandLineOption airAbsorptionOpt("air-absorption", "Air absorption on or off.", "state");
    QCommandLineOption dgPolyOpt("dg-poly-order", "DG polynomial order.", "order");
    QCommandLineOption dgFreqOpt("dg-max-frequency", "DG maximum frequency.", "hz");
    QCommandLineOption sampleRateOpt("sample-rate", "Override sample rate.", "hz");

    parser.addOption(projectOpt);
    parser.addOption(outputOpt);
    parser.addOption(methodOpt);
    parser.addOption(maxOrderOpt);
    parser.addOption(nRaysOpt);
    parser.addOption(scatteringOpt);
    parser.addOption(airAbsorptionOpt);
    parser.addOption(dgPolyOpt);
    parser.addOption(dgFreqOpt);
    parser.addOption(sampleRateOpt);

    parser.process(app);

    if (!parser.isSet(projectOpt)) {
        printError("Missing required --project <file.room> argument.");
        return 1;
    }

    const QString projectPath = QFileInfo(parser.value(projectOpt)).absoluteFilePath();
    auto project = prs::ProjectFile::load(projectPath);
    if (!project) {
        printError(QString("Failed to load project: %1").arg(projectPath));
        return 1;
    }

    prs::RenderOptions options;
    if (parser.isSet(methodOpt) && !parseMethod(parser.value(methodOpt), &options.method)) {
        printError("Invalid --method value. Use ray, dg2d, or dg3d.");
        return 1;
    }
    if (parser.isSet(maxOrderOpt))
        options.maxOrder = parser.value(maxOrderOpt).toInt();
    if (parser.isSet(nRaysOpt))
        options.nRays = parser.value(nRaysOpt).toInt();
    if (parser.isSet(scatteringOpt))
        options.scattering = parser.value(scatteringOpt).toFloat();
    if (parser.isSet(airAbsorptionOpt) && !parseOnOff(parser.value(airAbsorptionOpt), &options.airAbsorption)) {
        printError("Invalid --air-absorption value. Use on or off.");
        return 1;
    }
    if (parser.isSet(dgPolyOpt))
        options.dgPolyOrder = parser.value(dgPolyOpt).toInt();
    if (parser.isSet(dgFreqOpt))
        options.dgMaxFrequency = parser.value(dgFreqOpt).toFloat();
    if (parser.isSet(sampleRateOpt))
        options.sampleRate = parser.value(sampleRateOpt).toInt();

    const QString outputDir = parser.isSet(outputOpt) ? QFileInfo(parser.value(outputOpt)).absoluteFilePath() : QString();

    prs::SimulationWorker::Params params;
    QString error;
    if (!prs::RenderPipeline::buildSimulationParams(projectPath, *project, options, {}, outputDir, &params, &error)) {
        printError(error.isEmpty() ? QString("Failed to prepare render parameters for %1").arg(projectPath) : error);
        return 1;
    }

    prs::SimulationWorker worker(params);
    QString workerError;
    QObject::connect(&worker, &prs::SimulationWorker::error, [&](const QString& message) { workerError = message; });
    worker.process();

    if (!workerError.isEmpty()) {
        printError(workerError);
        return 1;
    }

    return 0;
}
