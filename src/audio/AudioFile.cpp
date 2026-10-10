#include "AudioFile.h"

#include <QFile>
#include <QDataStream>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <cmath>

#ifdef HAS_SNDFILE
#include <sndfile.h>
#endif

namespace prs {

#ifdef HAS_SNDFILE

bool AudioFile::load(const QString& filepath) {
    SF_INFO sfinfo;
    std::memset(&sfinfo, 0, sizeof(sfinfo));

    SNDFILE* file = sf_open(filepath.toStdString().c_str(), SFM_READ, &sfinfo);
    if (!file) return false;

    sampleRate_ = sfinfo.samplerate;
    channels_   = sfinfo.channels;

    std::vector<float> raw(sfinfo.frames * sfinfo.channels);
    sf_readf_float(file, raw.data(), sfinfo.frames);
    sf_close(file);

    samples_ = std::move(raw);
    if (channels_ > 1) convertToMono();
    return true;
}

bool AudioFile::save(const QString& filepath, int sampleRate) const {
    SF_INFO sfinfo;
    std::memset(&sfinfo, 0, sizeof(sfinfo));
    sfinfo.samplerate = sampleRate;
    sfinfo.channels   = 1;
    sfinfo.format     = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* file = sf_open(filepath.toStdString().c_str(), SFM_WRITE, &sfinfo);
    if (!file) return false;

    sf_writef_float(file, samples_.data(), static_cast<sf_count_t>(samples_.size()));
    sf_close(file);
    return true;
}

bool AudioFile::saveStereo(const QString& filepath, int sampleRate,
                          const std::vector<float>& left,
                          const std::vector<float>& right)
{
    if (left.size() != right.size() || left.empty()) return false;
    SF_INFO sfinfo;
    std::memset(&sfinfo, 0, sizeof(sfinfo));
    sfinfo.samplerate = sampleRate;
    sfinfo.channels   = 2;
    sfinfo.format     = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* file = sf_open(filepath.toStdString().c_str(), SFM_WRITE, &sfinfo);
    if (!file) return false;

    std::vector<float> interleaved(left.size() * 2);
    for (size_t i = 0; i < left.size(); ++i) {
        interleaved[i * 2]     = left[i];
        interleaved[i * 2 + 1] = right[i];
    }
    sf_writef_float(file, interleaved.data(), static_cast<sf_count_t>(left.size()));
    sf_close(file);
    return true;
}

#else

// Minimal WAV reader/writer when libsndfile is not available

bool AudioFile::load(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 44) return false;

    const char* ptr = data.constData();

    // Validate RIFF header
    if (std::memcmp(ptr, "RIFF", 4) != 0) return false;
    if (std::memcmp(ptr + 8, "WAVE", 4) != 0) return false;

    // Find fmt chunk
    int pos = 12;
    int16_t audioFormat = 0;
    int16_t numChannels = 0;
    int32_t sr = 0;
    int16_t bitsPerSample = 0;

    while (pos < data.size() - 8) {
        char chunkId[5] = {};
        std::memcpy(chunkId, ptr + pos, 4);
        int32_t chunkSize = *reinterpret_cast<const int32_t*>(ptr + pos + 4);

        if (std::strcmp(chunkId, "fmt ") == 0) {
            audioFormat   = *reinterpret_cast<const int16_t*>(ptr + pos + 8);
            numChannels   = *reinterpret_cast<const int16_t*>(ptr + pos + 10);
            sr            = *reinterpret_cast<const int32_t*>(ptr + pos + 12);
            bitsPerSample = *reinterpret_cast<const int16_t*>(ptr + pos + 22);
        } else if (std::strcmp(chunkId, "data") == 0) {
            sampleRate_ = sr;
            channels_   = numChannels;

            int bytesPerSample = bitsPerSample / 8;
            int totalSamples   = chunkSize / bytesPerSample;
            samples_.resize(totalSamples);

            const char* samplePtr = ptr + pos + 8;

            if (bitsPerSample == 16) {
                for (int i = 0; i < totalSamples; ++i) {
                    int16_t val = *reinterpret_cast<const int16_t*>(samplePtr + i * 2);
                    samples_[i] = val / 32768.0f;
                }
            } else if (bitsPerSample == 32 && audioFormat == 1) {
                for (int i = 0; i < totalSamples; ++i) {
                    int32_t val = *reinterpret_cast<const int32_t*>(samplePtr + i * 4);
                    samples_[i] = val / 2147483648.0f;
                }
            } else if (bitsPerSample == 32 && audioFormat == 3) {
                for (int i = 0; i < totalSamples; ++i) {
                    samples_[i] = *reinterpret_cast<const float*>(samplePtr + i * 4);
                }
            } else {
                return false;
            }

            if (channels_ > 1) convertToMono();
            return true;
        }

        pos += 8 + chunkSize;
        if (chunkSize % 2 != 0) ++pos;
    }

    return false;
}

// 16-bit PCM WAV; `interleaved` holds channels * frames samples in [-1, 1].
static bool writeWav16(const QString& filepath, int sampleRate, int16_t channels,
                       const std::vector<float>& interleaved) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    const int dataSize = static_cast<int>(interleaved.size()) * 2;

    struct {
        char     riff[4]       = {'R','I','F','F'};
        int32_t  fileSize      = 0;
        char     wave[4]       = {'W','A','V','E'};
        char     fmt[4]        = {'f','m','t',' '};
        int32_t  fmtSize       = 16;
        int16_t  audioFormat   = 1;
        int16_t  numChannels   = 1;
        int32_t  sampleRate    = 0;
        int32_t  byteRate      = 0;
        int16_t  blockAlign    = 2;
        int16_t  bitsPerSample = 16;
        char     data[4]       = {'d','a','t','a'};
        int32_t  dataSize      = 0;
    } header;

    header.fileSize    = 36 + dataSize;
    header.numChannels = channels;
    header.sampleRate  = sampleRate;
    header.byteRate    = sampleRate * 2 * channels;
    header.blockAlign  = static_cast<int16_t>(2 * channels);
    header.dataSize    = dataSize;

    std::vector<int16_t> pcm(interleaved.size());
    for (size_t i = 0; i < interleaved.size(); ++i)
        pcm[i] = static_cast<int16_t>(std::clamp(interleaved[i], -1.0f, 1.0f) * 32767.0f);

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(pcm.data()), dataSize);
    file.close();
    return true;
}

bool AudioFile::save(const QString& filepath, int sampleRate) const {
    return writeWav16(filepath, sampleRate, 1, samples_);
}

bool AudioFile::saveStereo(const QString& filepath, int sampleRate,
                          const std::vector<float>& left,
                          const std::vector<float>& right)
{
    if (left.size() != right.size() || left.empty()) return false;
    std::vector<float> interleaved(left.size() * 2);
    for (size_t i = 0; i < left.size(); ++i) {
        interleaved[2 * i]     = left[i];
        interleaved[2 * i + 1] = right[i];
    }
    return writeWav16(filepath, sampleRate, 2, interleaved);
}

#endif // HAS_SNDFILE

void AudioFile::convertToMono() {
    if (channels_ <= 1) return;

    int frames = static_cast<int>(samples_.size()) / channels_;
    std::vector<float> mono(frames);
    for (int i = 0; i < frames; ++i) {
        float sum = 0.0f;
        for (int c = 0; c < channels_; ++c)
            sum += samples_[i * channels_ + c];
        mono[i] = sum / channels_;
    }
    samples_ = std::move(mono);
    channels_ = 1;
}

void AudioFile::applyVolume(float volume) {
    for (auto& s : samples_)
        s *= volume;
}

} // namespace prs
