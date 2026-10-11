#pragma once

#include "DGTypes.h"
#include "DGBasis2D.h"
#include "DGMesh2D.h"
#include "DGBasis3D.h"
#include "DGMesh3D.h"

#if SEICHE_ENABLE_GL_COMPUTE
#include <QOpenGLFunctions_4_3_Core>
#include <memory>
#include <vector>
#endif

#include <string>

#if SEICHE_ENABLE_GL_COMPUTE
class QOpenGLContext;
class QOffscreenSurface;
#endif

namespace prs {
namespace dg {

class DGGpuCompute {
public:
    DGGpuCompute();
    ~DGGpuCompute();

    bool initContext();
    bool isAvailable() const { return available_; }

    bool init2D(const Basis2D& basis, const Mesh2D& mesh);
    void resetFields2D();
    void computeRHS2D(float sourcePulse);
    void rkStageUpdate2D(float rk_a, float rk_b, float dt);
    void recordListenerPressure2D(int step); // writes ir[step] on the GPU, no CPU sync

    void setSource2D(int elem, const VecXd& weights);
    void setListener2D(int elem, const VecXd& weights);

    bool init3D(const Basis3D& basis, const Mesh3D& mesh);
    void resetFields3D();
    void computeRHS3D(float sourcePulse);
    void rkStageUpdate3D(float rk_a, float rk_b, float dt);
    void recordListenerPressure3D(int step);

    // Listener IR stays on the GPU during time stepping; read it back once at the end.
    void allocImpulseResponse(int nSteps);
    std::vector<float> readImpulseResponse(int nSteps);
    void finish(); // block until queued GPU work is done

    void setSource3D(int elem, const VecXd& weights);
    void setListener3D(int elem, const VecXd& weights);

    void cleanup();

    std::string lastError() const { return lastError_; }

private:
    bool available_ = false;
    std::string lastError_;

#if SEICHE_ENABLE_GL_COMPUTE
    bool compileProgram(GLuint& program, const std::string& source);
    GLuint createBuffer(GLenum usage, GLsizeiptr size, const void* data = nullptr);
    void uploadBuffer(GLuint buf, GLsizeiptr size, const void* data);
    void initUniforms(GLuint rhsProgram, int Np, int K, int nFields, GLint& locPulse);
    std::string generateRHSShader2D(int Np, int totalFaceNodes, int localSize);
    std::string generateRKShader();
    std::string generateReadbackShader();
    std::string generateRHSShader3D(int Np, int totalFaceNodes, int localSize);

    std::unique_ptr<QOffscreenSurface> surface_;
    std::unique_ptr<QOpenGLContext> context_;
    QOpenGLFunctions_4_3_Core* gl_ = nullptr;

    GLuint rkProgram_ = 0;
    GLuint readbackProgram_ = 0;
    GLuint irBuf_ = 0;
    GLint locRkA_ = -1, locRkB_ = -1, locDt_ = -1, locStep_ = -1;

    struct State2D {
        GLuint rhsProgram = 0;
        GLuint fieldBuf = 0;
        GLuint rhsBuf = 0;
        GLuint resBuf = 0;
        GLuint opBuf = 0;
        GLuint geoBuf = 0;
        GLuint surfBuf = 0;
        GLuint connBuf = 0;
        GLuint srcBuf = 0;
        int Np = 0, K = 0, Nfp = 0, Nfaces = 0, totalFaceNodes = 0;
        int localSize = 0;
        int sourceElem = 0;
        int listenerElem = 0;
        GLint locPulse = -1;
    } s2d_;

    struct State3D {
        GLuint rhsProgram = 0;
        GLuint fieldBuf = 0;
        GLuint rhsBuf = 0;
        GLuint resBuf = 0;
        GLuint opBuf = 0;
        GLuint geoBuf = 0;
        GLuint surfBuf = 0;
        GLuint connBuf = 0;
        GLuint srcBuf = 0;
        int Np = 0, K = 0, Nfp = 0, Nfaces = 0, totalFaceNodes = 0;
        int localSize = 0;
        int sourceElem = 0;
        int listenerElem = 0;
        GLint locPulse = -1;
    } s3d_;
#endif
};

} // namespace dg
} // namespace prs
