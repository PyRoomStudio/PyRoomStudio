#include "SignalProcessing.h"

#include "core/Material.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <valarray>

namespace prs {
namespace SignalProcessing {

void normalize(std::vector<float>& signal, float headroom) {
    float maxAbs = 0.0f;
    for (float s : signal)
        maxAbs = std::max(maxAbs, std::abs(s));

    if (maxAbs > 0.0f) {
        float scale = headroom / maxAbs;
        for (float& s : signal)
            s *= scale;
    }
}

std::vector<float> convolve(const std::vector<float>& signal, const std::vector<float>& impulse) {
    if (signal.empty() || impulse.empty())
        return {};

    size_t outLen = signal.size() + impulse.size() - 1;
    std::vector<float> result(outLen, 0.0f);

    for (size_t i = 0; i < signal.size(); ++i) {
        for (size_t j = 0; j < impulse.size(); ++j) {
            result[i + j] += signal[i] * impulse[j];
        }
    }
    return result;
}

// Simple radix-2 Cooley-Tukey FFT
using Complex = std::complex<float>;

static void fft(std::valarray<Complex>& x) {
    size_t N = x.size();
    if (N <= 1)
        return;

    std::valarray<Complex> even = x[std::slice(0, N / 2, 2)];
    std::valarray<Complex> odd = x[std::slice(1, N / 2, 2)];

    fft(even);
    fft(odd);

    for (size_t k = 0; k < N / 2; ++k) {
        Complex t = std::polar(1.0f, -2.0f * static_cast<float>(M_PI) * k / N) * odd[k];
        x[k] = even[k] + t;
        x[k + N / 2] = even[k] - t;
    }
}

static void ifft(std::valarray<Complex>& x) {
    for (auto& v : x)
        v = std::conj(v);
    fft(x);
    for (auto& v : x)
        v = std::conj(v) / static_cast<float>(x.size());
}

static size_t nextPow2(size_t n) {
    size_t p = 1;
    while (p < n)
        p <<= 1;
    return p;
}

std::vector<float> fftConvolve(const std::vector<float>& signal, const std::vector<float>& impulse) {
    if (signal.empty() || impulse.empty())
        return {};

#ifdef HAS_FFTW3
    // FFTW-based implementation can be added here for better performance
    return convolve(signal, impulse);
#endif

    size_t outLen = signal.size() + impulse.size() - 1;
    size_t fftLen = nextPow2(outLen);

    std::valarray<Complex> sigFFT(fftLen);
    std::valarray<Complex> impFFT(fftLen);

    for (size_t i = 0; i < signal.size(); ++i)
        sigFFT[i] = Complex(signal[i], 0.0f);
    for (size_t i = 0; i < impulse.size(); ++i)
        impFFT[i] = Complex(impulse[i], 0.0f);

    fft(sigFFT);
    fft(impFFT);

    std::valarray<Complex> product = sigFFT * impFFT;
    ifft(product);

    std::vector<float> result(outLen);
    for (size_t i = 0; i < outLen; ++i)
        result[i] = product[i].real();

    return result;
}

std::vector<float> bandpassFilter(const std::vector<float>& signal, int sampleRate, float centerFreq) {
    if (signal.empty())
        return {};

    // 2nd-order Butterworth bandpass via biquad
    // Octave bandwidth: lower = center/sqrt(2), upper = center*sqrt(2)
    float lowFreq = centerFreq / std::sqrt(2.0f);
    float highFreq = centerFreq * std::sqrt(2.0f);

    // Clamp to Nyquist
    float nyquist = sampleRate / 2.0f;
    if (highFreq >= nyquist * 0.99f)
        highFreq = nyquist * 0.99f;
    if (lowFreq < 1.0f)
        lowFreq = 1.0f;
    if (lowFreq >= highFreq) {
        // Passthrough for degenerate case
        return signal;
    }

    float w0 = 2.0f * static_cast<float>(M_PI) * centerFreq / sampleRate;
    float bw = std::log2(highFreq / lowFreq);
    float alpha = std::sin(w0) * std::sinh(std::log(2.0f) / 2.0f * bw * w0 / std::sin(w0));

    float b0 = alpha;
    float b1 = 0.0f;
    float b2 = -alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * std::cos(w0);
    float a2 = 1.0f - alpha;

    // Normalize
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;

    std::vector<float> out(signal.size(), 0.0f);
    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
    for (size_t i = 0; i < signal.size(); ++i) {
        float x0 = signal[i];
        float y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        out[i] = y0;
        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = y0;
    }

    return out;
}

std::vector<float> combineMultibandRIR(const std::array<std::vector<float>, NUM_FREQ_BANDS>& bandRIRs, int sampleRate) {

    size_t maxLen = 0;
    for (auto& rir : bandRIRs)
        maxLen = std::max(maxLen, rir.size());

    if (maxLen == 0)
        return {};

    std::vector<float> combined(maxLen, 0.0f);

    for (int b = 0; b < NUM_FREQ_BANDS; ++b) {
        if (bandRIRs[b].empty())
            continue;

        float centerFreq = static_cast<float>(FREQ_BANDS[b]);
        auto filtered = bandpassFilter(bandRIRs[b], sampleRate, centerFreq);

        for (size_t i = 0; i < filtered.size() && i < maxLen; ++i)
            combined[i] += filtered[i];
    }

    return combined;
}

} // namespace SignalProcessing
} // namespace prs
