#pragma once

#include "DGBasis2D.h"
#include "DGBasis3D.h"
#include "DGMesh2D.h"
#include "DGMesh3D.h"
#include "DGTypes.h"

#include <memory>
#include <string>

#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__) && !defined(SEICHE_WEB_BUILD)
#include <QOpenGLFunctions_4_3_Core>
#endif

class QOpenGLContext;
class QOffscreenSurface;

namespace prs {
namespace dg {

#if defined(__APPLE__) || defined(__EMSCRIPTEN__) || defined(SEICHE_WEB_BUILD)

// macOS only supports OpenGL 4.1; compute shaders and SSBOs require 4.3+.
// WebAssembly/WebGL2 has no compute shaders at all.
// This stub always reports unavailable so callers fall back to CPU.
class DGGpuCompute {
  public:
    DGGpuCompute() = default;
    ~DGGpuCompute() = default;

    bool initContext() {
        lastError_ = "GPU compute unavailable on macOS (requires OpenGL 4.3+)";
        return false;
    }
    bool isAvailable() const { return false; }

    bool init2D(const Basis2D&, const Mesh2D&) { return false; }
    void resetFields2D() {}
    void computeRHS2D(float) {}
    void rkStageUpdate2D(float, float, float) {}
    float readListenerPressure2D() { return 0.0f; }
    void setSource2D(int, const VecXd&) {}
    void setListener2D(int, const VecXd&) {}

    bool init3D(const Basis3D&, const Mesh3D&) { return false; }
    void resetFields3D() {}
    void computeRHS3D(float) {}
    void rkStageUpdate3D(float, float, float) {}
    float readListenerPressure3D() { return 0.0f; }
    void setSource3D(int, const VecXd&) {}
    void setListener3D(int, const VecXd&) {}

    void cleanup() {}
    std::string lastError() const { return lastError_; }

  private:
    std::string lastError_;
};

#else  // native desktop (not macOS, not WASM)

class DGGpuCompute {
  public:
    DGGpuCompute();
    ~DGGpuCompute();

    bool initContext();
    bool isAvailable() const { return available_; }

    // 2D solver: upload mesh/basis data, run full time-stepping on GPU
    bool init2D(const Basis2D& basis, const Mesh2D& mesh);
    void resetFields2D();
    void computeRHS2D(float sourcePulse);
    void rkStageUpdate2D(float rk_a, float rk_b, float dt);
    float readListenerPressure2D();

    void setSource2D(int elem, const VecXd& weights);
    void setListener2D(int elem, const VecXd& weights);

    // 3D solver
    bool init3D(const Basis3D& basis, const Mesh3D& mesh);
    void resetFields3D();
    void computeRHS3D(float sourcePulse);
    void rkStageUpdate3D(float rk_a, float rk_b, float dt);
    float readListenerPressure3D();

    void setSource3D(int elem, const VecXd& weights);
    void setListener3D(int elem, const VecXd& weights);

    void cleanup();

    std::string lastError() const { return lastError_; }

  private:
    bool compileProgram(GLuint& program, const std::string& source);
    GLuint createBuffer(GLenum usage, GLsizeiptr size, const void* data = nullptr);
    void uploadBuffer(GLuint buf, GLsizeiptr size, const void* data);
    std::string generateRHSShader2D(int Np, int totalFaceNodes, int localSize);
    std::string generateRKShader();
    std::string generateReadbackShader();
    std::string generateRHSShader3D(int Np, int totalFaceNodes, int localSize);

    bool available_ = false;
    std::string lastError_;

    std::unique_ptr<QOffscreenSurface> surface_;
    std::unique_ptr<QOpenGLContext> context_;
    QOpenGLFunctions_4_3_Core* gl_ = nullptr;

    // Shared programs
    GLuint rkProgram_ = 0;
    GLuint readbackProgram_ = 0;

    // 2D-specific
    struct State2D {
        GLuint rhsProgram = 0;
        GLuint fieldBuf = 0;    // p, ux, uy packed (3 * Np * K floats)
        GLuint rhsBuf = 0;      // rhsP, rhsUx, rhsUy (3 * Np * K)
        GLuint resBuf = 0;      // resP, resUx, resUy (3 * Np * K)
        GLuint opBuf = 0;       // Dr, Ds, LIFT packed
        GLuint geoBuf = 0;      // rx, ry, sx, sy (4 * Np * K)
        GLuint surfBuf = 0;     // nx, ny, Fscale, boundaryImpedance
        GLuint connBuf = 0;     // vmapM, vmapP, EToE, EToF (int arrays)
        GLuint srcBuf = 0;      // source weights + listener weights + readback
        GLuint physicsBuf = 0;  // UBO for constants
        GLuint rkParamsBuf = 0; // UBO for RK params
        int Np = 0, K = 0, Nfp = 0, Nfaces = 0, totalFaceNodes = 0;
        int localSize = 0;
        int sourceElem = 0;
        int listenerElem = 0;
    } s2d_;

    // 3D-specific
    struct State3D {
        GLuint rhsProgram = 0;
        GLuint fieldBuf = 0; // p, ux, uy, uz (4 * Np * K)
        GLuint rhsBuf = 0;   // 4 * Np * K
        GLuint resBuf = 0;   // 4 * Np * K
        GLuint opBuf = 0;    // Dr, Ds, Dt, LIFT
        GLuint geoBuf = 0;   // rx,ry,rz, sx,sy,sz, tx,ty,tz (9 * Np * K)
        GLuint surfBuf = 0;  // nx,ny,nz, Fscale, boundaryImpedance
        GLuint connBuf = 0;  // vmapM, vmapP, EToE, EToF
        GLuint srcBuf = 0;
        GLuint physicsBuf = 0;
        GLuint rkParamsBuf = 0;
        int Np = 0, K = 0, Nfp = 0, Nfaces = 0, totalFaceNodes = 0;
        int localSize = 0;
        int sourceElem = 0;
        int listenerElem = 0;
    } s3d_;
};

#endif // desktop GPU compute guard

} // namespace dg
} // namespace prs
