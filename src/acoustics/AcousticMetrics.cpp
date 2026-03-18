#include "AcousticMetrics.h"

#include <cmath>
#include <algorithm>
#include <numeric>

namespace prs {
namespace AcousticMetrics {

std::vector<float> computeSchroederCurve(const std::vector<float>& rir) {
    if (rir.empty()) return {};

    std::vector<float> squared(rir.size());
    for (size_t i = 0; i < rir.size(); ++i)
        squared[i] = rir[i] * rir[i];

    // Backward integration (Schroeder method)
    std::vector<float> edc(rir.size());
    double runningSum = 0.0;
    for (int i = static_cast<int>(rir.size()) - 1; i >= 0; --i) {
        runningSum += squared[i];
        edc[i] = static_cast<float>(runningSum);
    }

    return edc;
}

std::vector<float> schroederCurveDb(const std::vector<float>& rir) {
    auto edc = computeSchroederCurve(rir);
    if (edc.empty()) return {};

    float maxVal = edc[0];
    if (maxVal < 1e-30f) return std::vector<float>(edc.size(), -200.0f);

    std::vector<float> edcDb(edc.size());
    for (size_t i = 0; i < edc.size(); ++i) {
        float ratio = edc[i] / maxVal;
        edcDb[i] = (ratio > 1e-30f) ? 10.0f * std::log10(ratio) : -300.0f;
    }
    return edcDb;
}

static float fitRT(const std::vector<float>& edcDb, int sampleRate,
                    float startDb, float endDb) {
    if (edcDb.empty() || sampleRate <= 0) return 0.0f;

    int startIdx = -1, endIdx = -1;
    for (int i = 0; i < static_cast<int>(edcDb.size()); ++i) {
        if (startIdx < 0 && edcDb[i] <= startDb)
            startIdx = i;
        if (startIdx >= 0 && edcDb[i] <= endDb) {
            endIdx = i;
            break;
        }
    }

    if (startIdx < 0 || endIdx < 0 || endIdx <= startIdx)
        return 0.0f;

    // Least-squares linear fit: edcDb[i] = slope * t[i] + intercept
    int n = endIdx - startIdx + 1;
    double sumT = 0, sumY = 0, sumTT = 0, sumTY = 0;
    for (int i = startIdx; i <= endIdx; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double y = edcDb[i];
        sumT  += t;
        sumY  += y;
        sumTT += t * t;
        sumTY += t * y;
    }

    double denom = n * sumTT - sumT * sumT;
    if (std::abs(denom) < 1e-30) return 0.0f;

    double slope = (n * sumTY - sumT * sumY) / denom;
    if (slope >= 0.0) return 0.0f;

    // Extrapolate to -60 dB
    return static_cast<float>(-60.0 / slope);
}

RTResult computeRT(const std::vector<float>& rir, int sampleRate) {
    auto edcDb = schroederCurveDb(rir);
    if (edcDb.empty()) return {};

    RTResult result;
    result.t20 = fitRT(edcDb, sampleRate, -5.0f, -25.0f);
    result.t30 = fitRT(edcDb, sampleRate, -5.0f, -35.0f);
    result.edt = fitRT(edcDb, sampleRate, 0.0f, -10.0f);
    result.valid = (result.t20 > 0.0f || result.t30 > 0.0f);
    return result;
}

SPLResult computeSPL(
    float sourceVolumeLinear,
    const std::vector<ImageSource>& imageSources,
    const std::vector<RayContribution>& rayContributions,
    int numRaysEmitted)
{
    SPLResult result;
    if (imageSources.empty() && rayContributions.empty())
        return result;

    double directEnergy = 0.0;
    double reflectedEnergy = 0.0;

    for (const auto& is : imageSources) {
        double bandAvg = 0.0;
        for (int b = 0; b < NUM_FREQ_BANDS; ++b)
            bandAvg += is.attenuation[b];
        bandAvg /= NUM_FREQ_BANDS;
        double e = bandAvg * bandAvg;
        if (is.order == 0)
            directEnergy += e;
        else
            reflectedEnergy += e;
    }

    double rayEnergy = 0.0;
    for (const auto& rc : rayContributions) {
        double bandAvg = 0.0;
        for (int b = 0; b < NUM_FREQ_BANDS; ++b)
            bandAvg += rc.energy[b];
        bandAvg /= NUM_FREQ_BANDS;
        rayEnergy += bandAvg * bandAvg;
    }
    if (numRaysEmitted > 0)
        rayEnergy *= (4.0 * M_PI) / numRaysEmitted;

    reflectedEnergy += rayEnergy;

    double volSq = static_cast<double>(sourceVolumeLinear) * sourceVolumeLinear;
    directEnergy *= volSq;
    reflectedEnergy *= volSq;

    double totalEnergy = directEnergy + reflectedEnergy;

    // Convert to dB (reference: energy of 1.0 at 1m from a unit source)
    constexpr double REF_ENERGY = 1e-12;
    result.directSPL    = (directEnergy > REF_ENERGY)
        ? static_cast<float>(10.0 * std::log10(directEnergy / REF_ENERGY)) : 0.0f;
    result.reflectedSPL = (reflectedEnergy > REF_ENERGY)
        ? static_cast<float>(10.0 * std::log10(reflectedEnergy / REF_ENERGY)) : 0.0f;
    result.totalSPL     = (totalEnergy > REF_ENERGY)
        ? static_cast<float>(10.0 * std::log10(totalEnergy / REF_ENERGY)) : 0.0f;
    result.valid = true;
    return result;
}

} // namespace AcousticMetrics
} // namespace prs
