#pragma once

#include "core/Types.h"

#include <QString>

#include <memory>
#include <optional>
#include <vector>

namespace prs {

struct HrtfSample {
    float azimuthDeg = 0.0f;
    float elevationDeg = 0.0f;
    int leftDelaySamples = 0;
    int rightDelaySamples = 0;
    std::vector<float> left;
    std::vector<float> right;
};

struct HrtfMetadata {
    QString name;
    QString format;
    QString sourcePath;
    int sampleRate = 0;
    int filterLength = 0;
    bool normalized = false;
};

class HrtfDataset {
  public:
    virtual ~HrtfDataset() = default;

    virtual HrtfMetadata metadata() const = 0;
    virtual bool validate(QString* error = nullptr) const = 0;
    virtual std::optional<HrtfSample> lookup(float azimuthDeg, float elevationDeg) const = 0;
};

class InMemoryHrtfDataset final : public HrtfDataset {
  public:
    InMemoryHrtfDataset() = default;
    InMemoryHrtfDataset(HrtfMetadata metadata, std::vector<HrtfSample> samples);

    HrtfMetadata metadata() const override;
    bool validate(QString* error = nullptr) const override;
    std::optional<HrtfSample> lookup(float azimuthDeg, float elevationDeg) const override;

    const std::vector<HrtfSample>& samples() const { return samples_; }
    std::vector<HrtfSample>& samples() { return samples_; }
    const HrtfMetadata& rawMetadata() const { return metadata_; }
    HrtfMetadata& rawMetadata() { return metadata_; }

  private:
    HrtfMetadata metadata_;
    std::vector<HrtfSample> samples_;
};

std::unique_ptr<HrtfDataset> makeSyntheticHrtfDataset();
std::unique_ptr<HrtfDataset> loadSofaHrtfDataset(const QString& path, int sampleRate, QString* error = nullptr);

} // namespace prs
