#pragma once

namespace prs {

class GpuCapabilities {
  public:
    static bool isGpuRayTracingSupported();
    static bool isGpuConvolutionSupported();
};

} // namespace prs
