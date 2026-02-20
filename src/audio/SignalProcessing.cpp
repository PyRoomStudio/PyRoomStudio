#include <cmath>
#include <algorithm>
#include <complex>
#include <valarray>

#include "SignalProcessing.h"

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

std::vector<float> convolve(const std::vector<float>& signal,
                            const std::vector<float>& impulse) {
    if (signal.empty() || impulse.empty()) return {};

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
    if (N <= 1) return;

    std::valarray<Complex> even = x[std::slice(0, N/2, 2)];
    std::valarray<Complex> odd  = x[std::slice(1, N/2, 2)];

    fft(even);
    fft(odd);

    for (size_t k = 0; k < N/2; ++k) {
        Complex t = std::polar(1.0f, -2.0f * static_cast<float>(M_PI) * k / N) * odd[k];
        x[k]       = even[k] + t;
        x[k + N/2] = even[k] - t;
    }
}

static void ifft(std::valarray<Complex>& x) {
    for (auto& v : x) v = std::conj(v);
    fft(x);
    for (auto& v : x) v = std::conj(v) / static_cast<float>(x.size());
}

static size_t nextPow2(size_t n) {
    size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

std::vector<float> fftConvolve(const std::vector<float>& signal,
                               const std::vector<float>& impulse) {
    if (signal.empty() || impulse.empty()) return {};

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

} // namespace SignalProcessing
} // namespace prs
