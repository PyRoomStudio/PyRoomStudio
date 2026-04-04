#include "GpuCapabilities.h"

#include <QOpenGLContext>
#include <QSurfaceFormat>

namespace prs {

bool GpuCapabilities::isGpuRayTracingSupported() {
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx)
        return false;

    const QSurfaceFormat fmt = ctx->format();
    const int major = fmt.majorVersion();
    const int minor = fmt.minorVersion();

    // Conservative check: require OpenGL 4.3+ for compute-shader-based kernels.
    if (major > 4)
        return true;
    if (major == 4 && minor >= 3)
        return true;

    return false;
}

bool GpuCapabilities::isGpuConvolutionSupported() {
    // For now, treat convolution support the same as general compute support.
    return isGpuRayTracingSupported();
}

} // namespace prs
