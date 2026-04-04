#include "SofaHrtfLoader.h"

#include <QFileInfo>
#include <QtGlobal>

#include <utility>

#ifdef HAS_LIBMYSOFA
#include <mysofa.h>
#endif

namespace prs {

namespace {

void setError(QString* error, const QString& message) {
    if (error)
        *error = message;
}

#ifdef HAS_LIBMYSOFA

class SofaHrtfDataset final : public HrtfDataset {
  public:
    SofaHrtfDataset(QString sourcePath, MYSOFA_EASY* handle, int sampleRate, int filterLength)
        : sourcePath_(std::move(sourcePath))
        , handle_(handle) {
        metadata_.name = QFileInfo(sourcePath_).fileName();
        metadata_.format = "sofa";
        metadata_.sourcePath = sourcePath_;
        metadata_.sampleRate = sampleRate;
        metadata_.filterLength = filterLength;
        metadata_.normalized = true;
    }

    ~SofaHrtfDataset() override {
        if (handle_)
            mysofa_close(handle_);
    }

    HrtfMetadata metadata() const override { return metadata_; }

    bool validate(QString* error = nullptr) const override {
        if (!handle_) {
            setError(error, "SOFA HRTF handle is null");
            return false;
        }
        if (metadata_.sampleRate <= 0) {
            setError(error, "SOFA HRTF sample rate must be positive");
            return false;
        }
        if (metadata_.filterLength <= 0) {
            setError(error, "SOFA HRTF filter length must be positive");
            return false;
        }
        return true;
    }

    std::optional<HrtfSample> lookup(float azimuthDeg, float elevationDeg) const override {
        if (!handle_)
            return std::nullopt;

        float coords[3] = {azimuthDeg, elevationDeg, 1.0f};
        mysofa_s2c(coords);

        std::vector<float> left(static_cast<size_t>(metadata_.filterLength));
        std::vector<float> right(static_cast<size_t>(metadata_.filterLength));
        float leftDelaySec = 0.0f;
        float rightDelaySec = 0.0f;
        mysofa_getfilter_float_nointerp(handle_, coords[0], coords[1], coords[2], left.data(), right.data(),
                                        &leftDelaySec, &rightDelaySec);

        HrtfSample sample;
        sample.azimuthDeg = azimuthDeg;
        sample.elevationDeg = elevationDeg;
        sample.left = std::move(left);
        sample.right = std::move(right);
        sample.leftDelaySamples = static_cast<int>(std::lround(leftDelaySec * metadata_.sampleRate));
        sample.rightDelaySamples = static_cast<int>(std::lround(rightDelaySec * metadata_.sampleRate));
        return sample;
    }

  private:
    QString sourcePath_;
    MYSOFA_EASY* handle_ = nullptr;
    HrtfMetadata metadata_;
};

#endif // HAS_LIBMYSOFA

} // namespace

std::unique_ptr<HrtfDataset> loadSofaHrtfDataset(const QString& path, int sampleRate, QString* error) {
#ifdef HAS_LIBMYSOFA
    int filterLength = 0;
    int err = 0;
    MYSOFA_EASY* handle = mysofa_open(path.toStdString().c_str(), sampleRate, &filterLength, &err);
    if (!handle) {
        setError(error, QString("failed to open SOFA HRTF dataset: %1 (mysofa error %2)").arg(path).arg(err));
        return nullptr;
    }

    auto dataset = std::make_unique<SofaHrtfDataset>(path, handle, sampleRate, filterLength);
    if (!dataset->validate(error)) {
        return nullptr;
    }
    return dataset;
#else
    Q_UNUSED(path);
    Q_UNUSED(sampleRate);
    setError(error, "libmysofa support is not available in this build");
    return nullptr;
#endif
}

} // namespace prs
