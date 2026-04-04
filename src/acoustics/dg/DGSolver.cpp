#include "DGSolver.h"

#include "audio/SignalProcessing.h"
#include "DGAcoustics2D.h"
#include "DGAcoustics3D.h"
#include "DGBasis2D.h"
#include "DGBasis3D.h"
#include "DGGpuCompute.h"
#include "DGMesh2D.h"
#include "DGMesh3D.h"

#include <QDebug>
#include <QElapsedTimer>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace prs {
namespace dg {

// LSRK4 coefficients (needed for GPU path)
static const float gpu_rk4a[5] = {0.0f, -567301805773.0f / 1357537059087.0f, -2404267990393.0f / 2016746695238.0f,
                                  -3550918686646.0f / 2091501179385.0f, -1275806237668.0f / 842570457699.0f};
static const float gpu_rk4b[5] = {1432997174477.0f / 9575080441755.0f, 5161836677717.0f / 13612068292357.0f,
                                  1720146321549.0f / 2090206949498.0f, 3134564353537.0f / 4481467310338.0f,
                                  2277821191437.0f / 14882151754819.0f};
static const float gpu_rk4c[5] = {0.0f, 1432997174477.0f / 9575080441755.0f, 2526269341429.0f / 6820363962896.0f,
                                  2006345519317.0f / 3224571057899.0f, 2802321613138.0f / 2924317926251.0f};

DGSolver::Result DGSolver::solve(const Vec3f& sourcePos, const Vec3f& listenerPos,
                                 const std::vector<Viewport3D::WallInfo>& walls,
                                 const std::vector<Vec3f>& modelVertices, const Bvh& bvh, const DGParams& params,
                                 std::function<void(int, const QString&)> progressCallback,
                                 std::function<bool()> cancelCheck) {
    if (params.method == DGMethod::DG_2D)
        return solve2D(sourcePos, listenerPos, walls, modelVertices, params, progressCallback, cancelCheck);
    else
        return solve3D(sourcePos, listenerPos, walls, modelVertices, bvh, params, progressCallback, cancelCheck);
}

DGSolver::Result DGSolver::solve2D(const Vec3f& sourcePos, const Vec3f& listenerPos,
                                   const std::vector<Viewport3D::WallInfo>& walls,
                                   const std::vector<Vec3f>& modelVertices, const DGParams& params,
                                   std::function<void(int, const QString&)> progressCallback,
                                   std::function<bool()> cancelCheck) {
    using Clock = std::chrono::steady_clock;
    QElapsedTimer timer;
    timer.start();

    if (progressCallback)
        progressCallback(0, "DG 2D: Building basis...");

    Basis2D basis = buildBasis2D(params.polynomialOrder);
    qInfo() << "DG 2D basis: N=" << params.polynomialOrder << "Np=" << basis.Np << "in" << timer.elapsed() << "ms";

    if (cancelCheck && cancelCheck())
        return {};

    double h = SOUND_SPEED / (params.maxFrequency * (2.0 * params.polynomialOrder + 1.0)) * 0.8;

    if (progressCallback)
        progressCallback(5, "DG 2D: Extracting room polygon...");

    float sliceZ = (sourcePos.z() + listenerPos.z()) * 0.5f;
    RoomPolygon2D polygon = extractRoomPolygon(walls, modelVertices, sliceZ);
    qInfo() << "Room polygon:" << polygon.vertices.size() << "vertices, z=" << sliceZ;

    if (progressCallback)
        progressCallback(10, QString("DG 2D: Meshing (h=%1m)...").arg(h, 0, 'f', 3));

    timer.restart();
    Mesh2D mesh = buildMesh2D(polygon, basis, h);
    qInfo() << "DG 2D mesh:" << mesh.K << "elements," << mesh.Nv << "vertices in" << timer.elapsed() << "ms";

    if (mesh.K == 0) {
        qWarning() << "DG 2D: Empty mesh, aborting";
        return {};
    }

    if (cancelCheck && cancelCheck())
        return {};

    // Try GPU acceleration
    DGGpuCompute gpu;
    bool useGpu = false;
    if (gpu.initContext() && gpu.init2D(basis, mesh)) {
        useGpu = true;
        qInfo() << "DG 2D: GPU compute shaders active";
        if (progressCallback)
            progressCallback(15, QString("DG 2D: GPU mode (%1 elems, N=%2)").arg(mesh.K).arg(params.polynomialOrder));
    } else {
        qInfo() << "DG 2D: GPU unavailable (" << QString::fromStdString(gpu.lastError())
                << "), using optimized CPU path";
        if (progressCallback)
            progressCallback(15, QString("DG 2D: CPU mode (%1 elems, N=%2)").arg(mesh.K).arg(params.polynomialOrder));
    }

    // Wrap progress for the inner solver (15% to 95%)
    auto solverProgress = [&](int pct, const std::string& msg) {
        if (progressCallback)
            progressCallback(15 + pct * 80 / 100, QString::fromStdString(msg));
    };
    auto solverCancel = [&]() -> bool { return cancelCheck && cancelCheck(); };

    timer.restart();
    Vec2d src2D(sourcePos.x(), sourcePos.y());
    Vec2d lis2D(listenerPos.x(), listenerPos.y());

    Result result;

    if (useGpu) {
        // --- GPU path: run entire time-stepping on GPU ---
        DGAcoustics2D cpuSolver(basis, mesh, params);

        int sourceElem = cpuSolver.findElement(src2D);
        int listenerElem = cpuSolver.findElement(lis2D);
        double dt = cpuSolver.computeTimeStep();
        double sourceSigma = 1.0 / (M_PI * params.maxFrequency);
        double sourceT0 = 4.0 * sourceSigma;

        // Source weights
        VecXd sourceWeights(basis.Np);
        {
            double totalW = 0.0;
            for (int i = 0; i < basis.Np; ++i) {
                double dx = mesh.x(i, sourceElem) - src2D.x();
                double dy = mesh.y(i, sourceElem) - src2D.y();
                double h2 = std::abs(mesh.J(0, sourceElem));
                sourceWeights(i) = std::exp(-(dx * dx + dy * dy) / (2.0 * h2 * 0.01));
                totalW += sourceWeights(i);
            }
            if (totalW > 1e-30)
                sourceWeights /= totalW;
        }

        VecXd listenerWeights(basis.Np);
        {
            double totalW = 0.0;
            for (int i = 0; i < basis.Np; ++i) {
                double dx = mesh.x(i, listenerElem) - lis2D.x();
                double dy = mesh.y(i, listenerElem) - lis2D.y();
                double w = 1.0 / (dx * dx + dy * dy + 1e-20);
                listenerWeights(i) = w;
                totalW += w;
            }
            listenerWeights /= totalW;
        }

        double simDuration = params.simulationDuration;
        if (simDuration <= 0.0) {
            double minX = mesh.VX.minCoeff(), maxX = mesh.VX.maxCoeff();
            double minY = mesh.VY.minCoeff(), maxY = mesh.VY.maxCoeff();
            double roomDiag = std::sqrt((maxX - minX) * (maxX - minX) + (maxY - minY) * (maxY - minY));
            simDuration = std::clamp(3.0 * roomDiag / SOUND_SPEED, 0.1, 2.0);
        }

        int nSteps = static_cast<int>(std::ceil(simDuration / dt));
        float dtF = static_cast<float>(dt);

        qInfo() << "DG 2D GPU: K=" << mesh.K << ", dt=" << (dt * 1e6) << "us,"
                << "steps=" << nSteps << ", duration=" << simDuration << "s";

        gpu.setSource2D(sourceElem, sourceWeights);
        gpu.setListener2D(listenerElem, listenerWeights);
        gpu.resetFields2D();

        std::vector<float> irData(nSteps, 0.0f);

        auto wallStart = Clock::now();
        double stepTimeSec = 0.0;
        int warmupSteps = std::min(50, nSteps);

        for (int step = 0; step < nSteps; ++step) {
            if (solverCancel())
                break;

            double time = step * dt;

            for (int stage = 0; stage < 5; ++stage) {
                double stageTime = time + static_cast<double>(gpu_rk4c[stage]) * dt;
                double arg = (stageTime - sourceT0) / sourceSigma;
                float pulse = static_cast<float>(std::exp(-0.5 * arg * arg));

                gpu.computeRHS2D(pulse);
                gpu.rkStageUpdate2D(gpu_rk4a[stage], gpu_rk4b[stage], dtF);
            }

            irData[step] = gpu.readListenerPressure2D();

            if (step == warmupSteps - 1) {
                auto now = Clock::now();
                stepTimeSec = std::chrono::duration<double>(now - wallStart).count() / warmupSteps;
            }

            if (step >= warmupSteps) {
                int reportEvery =
                    std::max(1, std::min(nSteps / 200, static_cast<int>(0.5 / std::max(stepTimeSec, 1e-9))));
                if (step % reportEvery == 0) {
                    int pct = static_cast<int>(100.0 * step / nSteps);
                    double remaining = stepTimeSec * (nSteps - step);
                    int etaMin = static_cast<int>(remaining / 60.0);
                    int etaSec = static_cast<int>(remaining) % 60;
                    solverProgress(pct, "DG 2D GPU: step " + std::to_string(step) + "/" + std::to_string(nSteps) +
                                            " [ETA " + std::to_string(etaMin) + "m" + std::to_string(etaSec) + "s]");
                }
            }
        }

        gpu.cleanup();

        AcousticsResult2D acousticResult;
        acousticResult.impulseResponse = std::move(irData);
        acousticResult.sampleRate = 1.0 / dt;
        acousticResult.duration = nSteps * dt;
        acousticResult.numTimeSteps = nSteps;

        qInfo() << "DG 2D GPU solve:" << nSteps << "steps in" << timer.elapsed() << "ms";

        result.duration = static_cast<float>(acousticResult.duration);
        result.impulseResponse = std::move(acousticResult.impulseResponse);
        result.sampleRate = static_cast<float>(acousticResult.sampleRate);
    } else {
        // --- CPU path ---
        DGAcoustics2D solver(basis, mesh, params);
        AcousticsResult2D acousticResult = solver.solve(src2D, lis2D, solverProgress, solverCancel);

        qInfo() << "DG 2D CPU solve:" << acousticResult.numTimeSteps << "steps,"
                << "dt=" << (1.0 / acousticResult.sampleRate * 1e6) << "us," << timer.elapsed() << "ms total";

        result.duration = static_cast<float>(acousticResult.duration);

        int targetSampleRate = DEFAULT_SAMPLE_RATE;
        if (std::abs(acousticResult.sampleRate - targetSampleRate) < 1.0) {
            result.impulseResponse = std::move(acousticResult.impulseResponse);
            result.sampleRate = static_cast<float>(acousticResult.sampleRate);
        } else {
            double ratio = static_cast<double>(targetSampleRate) / acousticResult.sampleRate;
            int outLen = static_cast<int>(acousticResult.impulseResponse.size() * ratio);
            result.impulseResponse.resize(outLen);
            for (int i = 0; i < outLen; ++i) {
                double srcIdx = i / ratio;
                int lo = static_cast<int>(srcIdx);
                int hi = lo + 1;
                double frac = srcIdx - lo;
                if (hi >= static_cast<int>(acousticResult.impulseResponse.size()))
                    hi = lo;
                result.impulseResponse[i] = static_cast<float>((1.0 - frac) * acousticResult.impulseResponse[lo] +
                                                               frac * acousticResult.impulseResponse[hi]);
            }
            result.sampleRate = static_cast<float>(targetSampleRate);
        }
    }

    // Normalize
    float maxVal = 0.0f;
    for (float v : result.impulseResponse)
        maxVal = std::max(maxVal, std::abs(v));
    if (maxVal > 1e-10f) {
        float scale = 1.0f / maxVal;
        for (float& v : result.impulseResponse)
            v *= scale;
    }

    // Resample from DG sample rate to audio rate if needed
    if (result.sampleRate > 0 && std::abs(result.sampleRate - DEFAULT_SAMPLE_RATE) > 1.0) {
        double ratio = static_cast<double>(DEFAULT_SAMPLE_RATE) / result.sampleRate;
        int outLen = static_cast<int>(result.impulseResponse.size() * ratio);
        std::vector<float> resampled(outLen);
        for (int i = 0; i < outLen; ++i) {
            double srcIdx = i / ratio;
            int lo = static_cast<int>(srcIdx);
            int hi = lo + 1;
            double frac = srcIdx - lo;
            if (hi >= static_cast<int>(result.impulseResponse.size()))
                hi = lo;
            resampled[i] =
                static_cast<float>((1.0 - frac) * result.impulseResponse[lo] + frac * result.impulseResponse[hi]);
        }
        result.impulseResponse = std::move(resampled);
        result.sampleRate = static_cast<float>(DEFAULT_SAMPLE_RATE);
    }

    if (progressCallback)
        progressCallback(100, "DG 2D: Complete");

    return result;
}

DGSolver::Result DGSolver::solve3D(const Vec3f& sourcePos, const Vec3f& listenerPos,
                                   const std::vector<Viewport3D::WallInfo>& walls,
                                   const std::vector<Vec3f>& modelVertices, const Bvh& bvh, const DGParams& params,
                                   std::function<void(int, const QString&)> progressCallback,
                                   std::function<bool()> cancelCheck) {
    using Clock = std::chrono::steady_clock;
    QElapsedTimer timer;
    timer.start();

    if (progressCallback)
        progressCallback(0, "DG 3D: Building basis...");

    Basis3D basis = buildBasis3D(params.polynomialOrder);
    qInfo() << "DG 3D basis: N=" << params.polynomialOrder << "Np=" << basis.Np << "in" << timer.elapsed() << "ms";

    if (cancelCheck && cancelCheck())
        return {};

    double h = SOUND_SPEED / (params.maxFrequency * (2.0 * params.polynomialOrder + 1.0)) * 0.8;

    if (progressCallback)
        progressCallback(5, QString("DG 3D: Meshing (h=%1m)...").arg(h, 0, 'f', 3));

    timer.restart();
    Mesh3D mesh = buildMesh3D(walls, modelVertices, bvh, basis, h);
    qInfo() << "DG 3D mesh:" << mesh.K << "elements," << mesh.Nv << "vertices in" << timer.elapsed() << "ms";

    if (mesh.K == 0) {
        qWarning() << "DG 3D: Empty mesh, aborting";
        return {};
    }

    if (cancelCheck && cancelCheck())
        return {};

    // Try GPU
    DGGpuCompute gpu;
    bool useGpu = false;
    if (gpu.initContext() && gpu.init3D(basis, mesh)) {
        useGpu = true;
        qInfo() << "DG 3D: GPU compute shaders active";
        if (progressCallback)
            progressCallback(15, QString("DG 3D: GPU mode (%1 elems, N=%2)").arg(mesh.K).arg(params.polynomialOrder));
    } else {
        qInfo() << "DG 3D: GPU unavailable (" << QString::fromStdString(gpu.lastError())
                << "), using optimized CPU path";
        if (progressCallback)
            progressCallback(15, QString("DG 3D: CPU mode (%1 elems, N=%2)").arg(mesh.K).arg(params.polynomialOrder));
    }

    auto solverProgress = [&](int pct, const std::string& msg) {
        if (progressCallback)
            progressCallback(15 + pct * 80 / 100, QString::fromStdString(msg));
    };
    auto solverCancel = [&]() -> bool { return cancelCheck && cancelCheck(); };

    timer.restart();
    Vec3d src3D(sourcePos.x(), sourcePos.y(), sourcePos.z());
    Vec3d lis3D(listenerPos.x(), listenerPos.y(), listenerPos.z());

    Result result;

    if (useGpu) {
        DGAcoustics3D cpuSolver(basis, mesh, params);

        int sourceElem = cpuSolver.findElement(src3D);
        int listenerElem = cpuSolver.findElement(lis3D);
        double dt = cpuSolver.computeTimeStep();
        double sourceSigma = 1.0 / (M_PI * params.maxFrequency);
        double sourceT0 = 4.0 * sourceSigma;

        VecXd sourceWeights(basis.Np);
        {
            double totalW = 0.0;
            for (int i = 0; i < basis.Np; ++i) {
                double dx = mesh.x(i, sourceElem) - src3D.x();
                double dy = mesh.y(i, sourceElem) - src3D.y();
                double dz = mesh.z(i, sourceElem) - src3D.z();
                double h2 = std::abs(mesh.J(0, sourceElem));
                sourceWeights(i) = std::exp(-(dx * dx + dy * dy + dz * dz) / (2.0 * h2 * 0.01));
                totalW += sourceWeights(i);
            }
            if (totalW > 1e-30)
                sourceWeights /= totalW;
        }

        VecXd listenerWeights(basis.Np);
        {
            double totalW = 0.0;
            for (int i = 0; i < basis.Np; ++i) {
                double dx = mesh.x(i, listenerElem) - lis3D.x();
                double dy = mesh.y(i, listenerElem) - lis3D.y();
                double dz = mesh.z(i, listenerElem) - lis3D.z();
                double w = 1.0 / (dx * dx + dy * dy + dz * dz + 1e-20);
                listenerWeights(i) = w;
                totalW += w;
            }
            listenerWeights /= totalW;
        }

        double simDuration = params.simulationDuration;
        if (simDuration <= 0.0) {
            double minX = mesh.VX.minCoeff(), maxX = mesh.VX.maxCoeff();
            double minY = mesh.VY.minCoeff(), maxY = mesh.VY.maxCoeff();
            double minZ = mesh.VZ.minCoeff(), maxZ = mesh.VZ.maxCoeff();
            double diag = std::sqrt((maxX - minX) * (maxX - minX) + (maxY - minY) * (maxY - minY) +
                                    (maxZ - minZ) * (maxZ - minZ));
            simDuration = std::clamp(3.0 * diag / SOUND_SPEED, 0.1, 2.0);
        }

        int nSteps = static_cast<int>(std::ceil(simDuration / dt));
        float dtF = static_cast<float>(dt);

        qInfo() << "DG 3D GPU: K=" << mesh.K << ", dt=" << (dt * 1e6) << "us,"
                << "steps=" << nSteps << ", duration=" << simDuration << "s";

        gpu.setSource3D(sourceElem, sourceWeights);
        gpu.setListener3D(listenerElem, listenerWeights);
        gpu.resetFields3D();

        std::vector<float> irData(nSteps, 0.0f);
        auto wallStart = Clock::now();
        double stepTimeSec = 0.0;
        int warmupSteps = std::min(50, nSteps);

        for (int step = 0; step < nSteps; ++step) {
            if (solverCancel())
                break;
            double time = step * dt;

            for (int stage = 0; stage < 5; ++stage) {
                double stageTime = time + static_cast<double>(gpu_rk4c[stage]) * dt;
                double arg = (stageTime - sourceT0) / sourceSigma;
                float pulse = static_cast<float>(std::exp(-0.5 * arg * arg));
                gpu.computeRHS3D(pulse);
                gpu.rkStageUpdate3D(gpu_rk4a[stage], gpu_rk4b[stage], dtF);
            }

            irData[step] = gpu.readListenerPressure3D();

            if (step == warmupSteps - 1) {
                auto now = Clock::now();
                stepTimeSec = std::chrono::duration<double>(now - wallStart).count() / warmupSteps;
            }
            if (step >= warmupSteps) {
                int reportEvery =
                    std::max(1, std::min(nSteps / 200, static_cast<int>(0.5 / std::max(stepTimeSec, 1e-9))));
                if (step % reportEvery == 0) {
                    int pct = static_cast<int>(100.0 * step / nSteps);
                    double remaining = stepTimeSec * (nSteps - step);
                    int etaMin = static_cast<int>(remaining / 60.0);
                    int etaSec = static_cast<int>(remaining) % 60;
                    solverProgress(pct, "DG 3D GPU: step " + std::to_string(step) + "/" + std::to_string(nSteps) +
                                            " [ETA " + std::to_string(etaMin) + "m" + std::to_string(etaSec) + "s]");
                }
            }
        }

        gpu.cleanup();

        result.duration = static_cast<float>(nSteps * dt);
        result.impulseResponse = std::move(irData);
        result.sampleRate = static_cast<float>(1.0 / dt);

        qInfo() << "DG 3D GPU solve:" << nSteps << "steps in" << timer.elapsed() << "ms";
    } else {
        DGAcoustics3D solver(basis, mesh, params);
        AcousticsResult3D acousticResult = solver.solve(src3D, lis3D, solverProgress, solverCancel);

        qInfo() << "DG 3D CPU solve:" << acousticResult.numTimeSteps << "steps,"
                << "dt=" << (1.0 / acousticResult.sampleRate * 1e6) << "us," << timer.elapsed() << "ms total";

        result.duration = static_cast<float>(acousticResult.duration);

        int targetSampleRate = DEFAULT_SAMPLE_RATE;
        if (std::abs(acousticResult.sampleRate - targetSampleRate) < 1.0) {
            result.impulseResponse = std::move(acousticResult.impulseResponse);
            result.sampleRate = static_cast<float>(acousticResult.sampleRate);
        } else {
            double ratio = static_cast<double>(targetSampleRate) / acousticResult.sampleRate;
            int outLen = static_cast<int>(acousticResult.impulseResponse.size() * ratio);
            result.impulseResponse.resize(outLen);
            for (int i = 0; i < outLen; ++i) {
                double srcIdx = i / ratio;
                int lo = static_cast<int>(srcIdx);
                int hi = lo + 1;
                double frac = srcIdx - lo;
                if (hi >= static_cast<int>(acousticResult.impulseResponse.size()))
                    hi = lo;
                result.impulseResponse[i] = static_cast<float>((1.0 - frac) * acousticResult.impulseResponse[lo] +
                                                               frac * acousticResult.impulseResponse[hi]);
            }
            result.sampleRate = static_cast<float>(targetSampleRate);
        }
    }

    // Normalize
    float maxVal = 0.0f;
    for (float v : result.impulseResponse)
        maxVal = std::max(maxVal, std::abs(v));
    if (maxVal > 1e-10f) {
        float scale = 1.0f / maxVal;
        for (float& v : result.impulseResponse)
            v *= scale;
    }

    // Resample if needed
    if (result.sampleRate > 0 && std::abs(result.sampleRate - DEFAULT_SAMPLE_RATE) > 1.0) {
        double ratio = static_cast<double>(DEFAULT_SAMPLE_RATE) / result.sampleRate;
        int outLen = static_cast<int>(result.impulseResponse.size() * ratio);
        std::vector<float> resampled(outLen);
        for (int i = 0; i < outLen; ++i) {
            double srcIdx = i / ratio;
            int lo = static_cast<int>(srcIdx);
            int hi = lo + 1;
            double frac = srcIdx - lo;
            if (hi >= static_cast<int>(result.impulseResponse.size()))
                hi = lo;
            resampled[i] =
                static_cast<float>((1.0 - frac) * result.impulseResponse[lo] + frac * result.impulseResponse[hi]);
        }
        result.impulseResponse = std::move(resampled);
        result.sampleRate = static_cast<float>(DEFAULT_SAMPLE_RATE);
    }

    if (progressCallback)
        progressCallback(100, "DG 3D: Complete");

    return result;
}

} // namespace dg
} // namespace prs
