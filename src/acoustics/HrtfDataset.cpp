#include "HrtfDataset.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <utility>

namespace prs {

namespace {

float wrapAzimuth(float azimuthDeg) {
    float wrapped = std::fmod(azimuthDeg, 360.0f);
    if (wrapped > 180.0f)
        wrapped -= 360.0f;
    if (wrapped <= -180.0f)
        wrapped += 360.0f;
    return wrapped;
}

float angularDistance(float queryAzimuthDeg, float queryElevationDeg, const HrtfSample& sample) {
    const float azimuthDelta = wrapAzimuth(queryAzimuthDeg - sample.azimuthDeg);
    const float elevationDelta = queryElevationDeg - sample.elevationDeg;
    return azimuthDelta * azimuthDelta + elevationDelta * elevationDelta;
}

bool sampleLooksValid(const HrtfSample& sample, int expectedLength) {
    return sample.left.size() == static_cast<size_t>(expectedLength) &&
           sample.right.size() == static_cast<size_t>(expectedLength);
}

void setError(QString* error, const QString& message) {
    if (error)
        *error = message;
}

} // namespace

InMemoryHrtfDataset::InMemoryHrtfDataset(HrtfMetadata metadata, std::vector<HrtfSample> samples)
    : metadata_(std::move(metadata))
    , samples_(std::move(samples)) {}

HrtfMetadata InMemoryHrtfDataset::metadata() const {
    return metadata_;
}

bool InMemoryHrtfDataset::validate(QString* error) const {
    if (metadata_.format.isEmpty()) {
        setError(error, "HRTF dataset format must be set");
        return false;
    }
    if (metadata_.sampleRate <= 0) {
        setError(error, "HRTF dataset sample rate must be positive");
        return false;
    }
    if (metadata_.filterLength <= 0) {
        setError(error, "HRTF dataset filter length must be positive");
        return false;
    }
    if (samples_.empty()) {
        setError(error, "HRTF dataset must contain at least one measurement");
        return false;
    }

    for (const auto& sample : samples_) {
        if (!sampleLooksValid(sample, metadata_.filterLength)) {
            setError(error, "HRTF dataset filter length does not match measurement data");
            return false;
        }
        if (sample.leftDelaySamples < 0 || sample.rightDelaySamples < 0) {
            setError(error, "HRTF dataset delays must be non-negative");
            return false;
        }
    }

    return true;
}

std::optional<HrtfSample> InMemoryHrtfDataset::lookup(float azimuthDeg, float elevationDeg) const {
    if (samples_.empty())
        return std::nullopt;

    const auto it = std::min_element(
        samples_.begin(), samples_.end(), [azimuthDeg, elevationDeg](const HrtfSample& a, const HrtfSample& b) {
            return angularDistance(azimuthDeg, elevationDeg, a) < angularDistance(azimuthDeg, elevationDeg, b);
        });
    if (it == samples_.end())
        return std::nullopt;
    return *it;
}

std::unique_ptr<HrtfDataset> makeSyntheticHrtfDataset() {
    HrtfMetadata metadata;
    metadata.name = "Synthetic HRTF";
    metadata.format = "synthetic";
    metadata.sampleRate = 48000;
    metadata.filterLength = 3;
    metadata.normalized = true;

    std::vector<HrtfSample> samples = {
        HrtfSample{0.0f, 0.0f, 0, 0, {1.0f, 0.0f, 0.0f}, {0.8f, 0.0f, 0.0f}},
        HrtfSample{90.0f, 0.0f, 0, 1, {0.2f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        HrtfSample{-90.0f, 0.0f, 1, 0, {1.0f, 0.0f, 0.0f}, {0.2f, 0.0f, 0.0f}},
    };

    return std::make_unique<InMemoryHrtfDataset>(std::move(metadata), std::move(samples));
}

} // namespace prs
