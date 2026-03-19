#include "DGGpuCompute.h"

#include <QOpenGLContext>
#include <QOpenGLVersionFunctionsFactory>
#include <QOffscreenSurface>
#include <QSurfaceFormat>
#include <QDebug>

#include <vector>
#include <cstring>
#include <sstream>

// macOS OpenGL headers (via the SDK/Qt wrappers) sometimes omit these SSBO / barrier constants,
// even when the functions exist. Provide safe fallback values so the project compiles.
#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif
#ifndef GL_SHADER_STORAGE_BARRIER_BIT
#define GL_SHADER_STORAGE_BARRIER_BIT 0x00002000
#endif
#ifndef GL_BUFFER_UPDATE_BARRIER_BIT
#define GL_BUFFER_UPDATE_BARRIER_BIT 0x00000200
#endif
#ifndef GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS
#define GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS 0x90DD
#endif
#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif

namespace prs {
namespace dg {

static int nextPow2(int v) {
    int p = 1;
    while (p < v) p <<= 1;
    return std::max(p, 32); // minimum warp size
}

DGGpuCompute::DGGpuCompute() = default;

DGGpuCompute::~DGGpuCompute() { cleanup(); }

bool DGGpuCompute::initContext() {
    QSurfaceFormat fmt;
    fmt.setVersion(4, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setRenderableType(QSurfaceFormat::OpenGL);

    surface_ = std::make_unique<QOffscreenSurface>();
    surface_->setFormat(fmt);
    surface_->create();
    if (!surface_->isValid()) {
        lastError_ = "Failed to create offscreen surface";
        return false;
    }

    context_ = std::make_unique<QOpenGLContext>();
    context_->setFormat(fmt);
    if (!context_->create()) {
        lastError_ = "Failed to create OpenGL 4.3 context";
        return false;
    }

    if (!context_->makeCurrent(surface_.get())) {
        lastError_ = "Failed to make GL context current";
        return false;
    }

    gl_ = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_3_Core>(context_.get());
    if (!gl_) {
        lastError_ = "OpenGL 4.3 functions not available";
        context_->doneCurrent();
        return false;
    }
    gl_->initializeOpenGLFunctions();

    GLint maxSSBO = 0;
    gl_->glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxSSBO);
    if (maxSSBO < 8) {
        lastError_ = "Need at least 8 SSBO bindings, have " + std::to_string(maxSSBO);
        return false;
    }

    qInfo() << "DG GPU compute: OpenGL" << context_->format().majorVersion()
            << "." << context_->format().minorVersion()
            << "| max SSBOs:" << maxSSBO;

    available_ = true;
    return true;
}

void DGGpuCompute::cleanup() {
    if (!gl_) return;

    auto deleteProgram = [&](GLuint& p) { if (p) { gl_->glDeleteProgram(p); p = 0; } };
    auto deleteBuf = [&](GLuint& b) { if (b) { gl_->glDeleteBuffers(1, &b); b = 0; } };

    deleteProgram(rkProgram_);
    deleteProgram(readbackProgram_);

    deleteProgram(s2d_.rhsProgram);
    deleteBuf(s2d_.fieldBuf); deleteBuf(s2d_.rhsBuf); deleteBuf(s2d_.resBuf);
    deleteBuf(s2d_.opBuf); deleteBuf(s2d_.geoBuf); deleteBuf(s2d_.surfBuf);
    deleteBuf(s2d_.connBuf); deleteBuf(s2d_.srcBuf);
    deleteBuf(s2d_.physicsBuf); deleteBuf(s2d_.rkParamsBuf);

    deleteProgram(s3d_.rhsProgram);
    deleteBuf(s3d_.fieldBuf); deleteBuf(s3d_.rhsBuf); deleteBuf(s3d_.resBuf);
    deleteBuf(s3d_.opBuf); deleteBuf(s3d_.geoBuf); deleteBuf(s3d_.surfBuf);
    deleteBuf(s3d_.connBuf); deleteBuf(s3d_.srcBuf);
    deleteBuf(s3d_.physicsBuf); deleteBuf(s3d_.rkParamsBuf);

    if (context_) context_->doneCurrent();
    available_ = false;
}

bool DGGpuCompute::compileProgram(GLuint& program, const std::string& source) {
    GLuint shader = gl_->glCreateShader(GL_COMPUTE_SHADER);
    const char* src = source.c_str();
    gl_->glShaderSource(shader, 1, &src, nullptr);
    gl_->glCompileShader(shader);

    GLint ok = 0;
    gl_->glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        gl_->glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        lastError_ = std::string("Shader compile: ") + log;
        gl_->glDeleteShader(shader);
        return false;
    }

    program = gl_->glCreateProgram();
    gl_->glAttachShader(program, shader);
    gl_->glLinkProgram(program);
    gl_->glDeleteShader(shader);

    gl_->glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        gl_->glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        lastError_ = std::string("Program link: ") + log;
        gl_->glDeleteProgram(program);
        program = 0;
        return false;
    }
    return true;
}

GLuint DGGpuCompute::createBuffer(GLenum usage, GLsizeiptr size, const void* data) {
    GLuint buf = 0;
    gl_->glGenBuffers(1, &buf);
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf);
    gl_->glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return buf;
}

void DGGpuCompute::uploadBuffer(GLuint buf, GLsizeiptr size, const void* data) {
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf);
    gl_->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

// ==================== 2D SHADER GENERATION ====================

std::string DGGpuCompute::generateRHSShader2D(int Np, int totalFaceNodes, int localSize) {
    std::ostringstream ss;
    ss << R"(#version 430
layout(local_size_x = )" << localSize << R"() in;

// SSBO 0: fields (p, ux, uy) packed [3 * Np * K]
layout(std430, binding = 0) readonly buffer Fields { float fields[]; };

// SSBO 1: RHS output [3 * Np * K]
layout(std430, binding = 1) buffer RHS { float rhs[]; };

// SSBO 2: operators (Dr[Np*Np], Ds[Np*Np], LIFT[Np*TFN])
layout(std430, binding = 3) readonly buffer Ops { float ops[]; };

// SSBO 3: geometry (rx, ry, sx, sy) [4 * Np * K]
layout(std430, binding = 4) readonly buffer Geo { float geo[]; };

// SSBO 4: surface (nx, ny, Fscale, boundaryImpedance) [4 * TFN * K]
layout(std430, binding = 5) readonly buffer Surf { float surf[]; };

// SSBO 5: connectivity
//   vmapM[TFN*K], vmapP[TFN*K], EToE[K*Nfaces], EToF[K*Nfaces]
layout(std430, binding = 6) readonly buffer Conn { int conn[]; };

// SSBO 6: source (sourceWeights[Np], sourceElem, sourcePulse)
layout(std430, binding = 7) readonly buffer Src { float srcData[]; };

const int cNp = )" << Np << R"(;
const int cNfaces = 3;
const int cNfp = )" << (totalFaceNodes / 3) << R"(;
const int cTFN = )" << totalFaceNodes << R"(;

uniform int uK;
uniform float uRho;
uniform float uC;

shared float s_p[)" << localSize << R"(];
shared float s_ux[)" << localSize << R"(];
shared float s_uy[)" << localSize << R"(];
shared float s_flux_p[)" << localSize << R"(];
shared float s_flux_ux[)" << localSize << R"(];
shared float s_flux_uy[)" << localSize << R"(];

void main() {
    int k = int(gl_WorkGroupID.x);
    int tid = int(gl_LocalInvocationID.x);

    if (k >= uK) return;

    float c2 = uC * uC;
    float invRho = 1.0 / uRho;
    float Z = uRho * uC;
    float invZ = 1.0 / Z;

    int fieldBase = k * cNp;

    // Load element field data
    if (tid < cNp) {
        s_p[tid]  = fields[fieldBase + tid];
        s_ux[tid] = fields[uK * cNp + fieldBase + tid];
        s_uy[tid] = fields[2 * uK * cNp + fieldBase + tid];
    }
    barrier();

    // Phase 1: Volume terms
    if (tid < cNp) {
        float dpdr = 0.0, dpds = 0.0;
        float duxdr = 0.0, duxds = 0.0;
        float duydr = 0.0, duyds = 0.0;

        int drBase = 0;
        int dsBase = cNp * cNp;

        for (int j = 0; j < cNp; ++j) {
            float dr_v = ops[drBase + tid * cNp + j];
            float ds_v = ops[dsBase + tid * cNp + j];
            dpdr  += dr_v * s_p[j];
            dpds  += ds_v * s_p[j];
            duxdr += dr_v * s_ux[j];
            duxds += ds_v * s_ux[j];
            duydr += dr_v * s_uy[j];
            duyds += ds_v * s_uy[j];
        }

        int geoBase = k * cNp + tid;
        float rxv = geo[geoBase];
        float ryv = geo[uK * cNp + geoBase];
        float sxv = geo[2 * uK * cNp + geoBase];
        float syv = geo[3 * uK * cNp + geoBase];

        float dpdx = rxv * dpdr + sxv * dpds;
        float dpdy = ryv * dpdr + syv * dpds;
        float divU = (rxv * duxdr + sxv * duxds) + (ryv * duydr + syv * duyds);

        int outIdx = fieldBase + tid;
        rhs[outIdx]                 = -uRho * c2 * divU;
        rhs[uK * cNp + outIdx]     = -invRho * dpdx;
        rhs[2 * uK * cNp + outIdx] = -invRho * dpdy;
    }
    barrier();

    // Phase 2: Surface flux
    if (tid < cTFN) {
        int fid = tid;
        int f = fid / cNfp;
        int surfIdx = k * cTFN + fid;

        int connBase_vmapM = 0;
        int connBase_vmapP = cTFN * uK;
        int connBase_EToE  = 2 * cTFN * uK;
        int connBase_EToF  = 2 * cTFN * uK + uK * cNfaces;

        int EToE_kf = conn[connBase_EToE + k * cNfaces + f];
        int EToF_kf = conn[connBase_EToF + k * cNfaces + f];
        bool isBoundary = (EToE_kf == k && EToF_kf == f);

        int idM = conn[connBase_vmapM + surfIdx];
        int idP = conn[connBase_vmapP + surfIdx];

        int kM = idM / cNp, nM = idM - kM * cNp;
        float pM  = fields[kM * cNp + nM];
        float uxM = fields[uK * cNp + kM * cNp + nM];
        float uyM = fields[2 * uK * cNp + kM * cNp + nM];

        float pP, uxP, uyP;

        int surfDataBase = k * cTFN + fid;
        float fnx = surf[surfDataBase];
        float fny = surf[uK * cTFN + surfDataBase];

        if (isBoundary) {
            float Zwall = surf[3 * uK * cTFN + surfIdx];
            if (Zwall < 1e-10) Zwall = 1e10;
            float R = (Zwall - Z) / (Zwall + Z);

            float unM = uxM * fnx + uyM * fny;
            float unP = -R * unM;
            pP = R * pM;
            float utxM = uxM - unM * fnx;
            float utyM = uyM - unM * fny;
            uxP = unP * fnx + utxM;
            uyP = unP * fny + utyM;
        } else {
            int kP = idP / cNp, nP = idP - kP * cNp;
            pP  = fields[kP * cNp + nP];
            uxP = fields[uK * cNp + kP * cNp + nP];
            uyP = fields[2 * uK * cNp + kP * cNp + nP];
        }

        float dp = pP - pM;
        float dunM = (uxP - uxM) * fnx + (uyP - uyM) * fny;
        float fsc = surf[2 * uK * cTFN + surfDataBase];

        s_flux_p[tid]  = 0.5 * (uC * invZ * dp - c2 * dunM) * fsc;
        float fluxUn   = 0.5 * (-invRho * dp + uC * Z * invRho * dunM) * fsc;
        s_flux_ux[tid] = fluxUn * fnx;
        s_flux_uy[tid] = fluxUn * fny;
    }
    barrier();

    // Phase 3: LIFT application + source term
    if (tid < cNp) {
        float liftP = 0.0, liftUx = 0.0, liftUy = 0.0;
        int liftBase = 2 * cNp * cNp;  // after Dr, Ds

        for (int j = 0; j < cTFN; ++j) {
            float l = ops[liftBase + tid * cTFN + j];
            liftP  += l * s_flux_p[j];
            liftUx += l * s_flux_ux[j];
            liftUy += l * s_flux_uy[j];
        }

        int outIdx = fieldBase + tid;
        rhs[outIdx]                 += liftP;
        rhs[uK * cNp + outIdx]     += liftUx;
        rhs[2 * uK * cNp + outIdx] += liftUy;

        int srcElem = int(srcData[cNp]);
        if (k == srcElem) {
            float pulse = srcData[cNp + 1];
            rhs[outIdx] += pulse * srcData[tid];
        }
    }
}
)";
    return ss.str();
}

std::string DGGpuCompute::generateRKShader() {
    return R"(#version 430
layout(local_size_x = 256) in;

// SSBO 0: fields (packed: N_FIELDS * Np * K)
layout(std430, binding = 0) buffer Fields { float fields[]; };
// SSBO 1: RHS
layout(std430, binding = 1) readonly buffer RHS { float rhs[]; };
// SSBO 2: residuals
layout(std430, binding = 2) buffer Res { float res[]; };

uniform float uRkA;
uniform float uRkB;
uniform float uDt;
uniform int uTotalNodes;  // N_FIELDS * Np * K

void main() {
    int idx = int(gl_GlobalInvocationID.x);
    if (idx >= uTotalNodes) return;

    float r = uRkA * res[idx] + uDt * rhs[idx];
    res[idx] = r;
    fields[idx] += uRkB * r;
}
)";
}

std::string DGGpuCompute::generateReadbackShader() {
    return R"(#version 430
layout(local_size_x = 1) in;

// SSBO 0: fields (p is at offset 0)
layout(std430, binding = 0) readonly buffer Fields { float fields[]; };
// SSBO 6: source/listener data (listener weights at offset Np+2, result at offset 2*Np+2)
layout(std430, binding = 7) buffer SrcData { float srcData[]; };

uniform int uListenerElem;
uniform int uNp;
uniform int uK;

void main() {
    float sum = 0.0;
    int weightsOffset = uNp + 2;  // after source weights + sourceElem + sourcePulse
    for (int i = 0; i < uNp; ++i) {
        sum += srcData[weightsOffset + i] * fields[uListenerElem * uNp + i];
    }
    // Write result at offset (2*Np + 2)
    srcData[2 * uNp + 2] = sum;
}
)";
}

// ==================== 2D INITIALIZATION ====================

bool DGGpuCompute::init2D(const Basis2D& basis, const Mesh2D& mesh) {
    if (!available_) return false;

    s2d_.Np = basis.Np;
    s2d_.K = mesh.K;
    s2d_.Nfp = basis.Nfp;
    s2d_.Nfaces = basis.Nfaces;
    s2d_.totalFaceNodes = basis.Nfp * basis.Nfaces;
    s2d_.localSize = nextPow2(std::max(basis.Np, s2d_.totalFaceNodes));

    // Compile shaders
    std::string rhsSrc = generateRHSShader2D(s2d_.Np, s2d_.totalFaceNodes, s2d_.localSize);
    if (!compileProgram(s2d_.rhsProgram, rhsSrc)) {
        qWarning() << "DG GPU: RHS shader failed:" << QString::fromStdString(lastError_);
        return false;
    }
    if (!rkProgram_) {
        if (!compileProgram(rkProgram_, generateRKShader())) {
            qWarning() << "DG GPU: RK shader failed:" << QString::fromStdString(lastError_);
            return false;
        }
    }
    if (!readbackProgram_) {
        if (!compileProgram(readbackProgram_, generateReadbackShader())) {
            qWarning() << "DG GPU: Readback shader failed:" << QString::fromStdString(lastError_);
            return false;
        }
    }

    int Np = s2d_.Np;
    int K = s2d_.K;
    int TFN = s2d_.totalFaceNodes;
    int Nfaces = s2d_.Nfaces;

    // Fields: 3 * Np * K floats (p, ux, uy)
    s2d_.fieldBuf = createBuffer(GL_DYNAMIC_DRAW, 3 * Np * K * sizeof(float));
    s2d_.rhsBuf   = createBuffer(GL_DYNAMIC_DRAW, 3 * Np * K * sizeof(float));
    s2d_.resBuf   = createBuffer(GL_DYNAMIC_DRAW, 3 * Np * K * sizeof(float));

    // Operators: Dr(Np*Np) + Ds(Np*Np) + LIFT(Np*TFN)
    {
        int opSize = Np * Np + Np * Np + Np * TFN;
        std::vector<float> opData(opSize);
        int off = 0;
        // Dr (row-major)
        for (int i = 0; i < Np; ++i)
            for (int j = 0; j < Np; ++j)
                opData[off++] = static_cast<float>(basis.Dr(i, j));
        // Ds
        for (int i = 0; i < Np; ++i)
            for (int j = 0; j < Np; ++j)
                opData[off++] = static_cast<float>(basis.Ds(i, j));
        // LIFT
        for (int i = 0; i < Np; ++i)
            for (int j = 0; j < TFN; ++j)
                opData[off++] = static_cast<float>(basis.LIFT(i, j));

        s2d_.opBuf = createBuffer(GL_STATIC_DRAW, opSize * sizeof(float), opData.data());
    }

    // Geometric factors: rx, ry, sx, sy (4 * Np * K), column-major (node + elem * Np)
    {
        int geoSize = 4 * Np * K;
        std::vector<float> geoData(geoSize);
        int off = 0;
        for (int k = 0; k < K; ++k)
            for (int i = 0; i < Np; ++i)
                geoData[off++] = static_cast<float>(mesh.rx(i, k));
        for (int k = 0; k < K; ++k)
            for (int i = 0; i < Np; ++i)
                geoData[off++] = static_cast<float>(mesh.ry(i, k));
        for (int k = 0; k < K; ++k)
            for (int i = 0; i < Np; ++i)
                geoData[off++] = static_cast<float>(mesh.sx(i, k));
        for (int k = 0; k < K; ++k)
            for (int i = 0; i < Np; ++i)
                geoData[off++] = static_cast<float>(mesh.sy(i, k));

        s2d_.geoBuf = createBuffer(GL_STATIC_DRAW, geoSize * sizeof(float), geoData.data());
    }

    // Surface: nx, ny, Fscale, boundaryImpedance (4 * TFN * K)
    {
        int surfSize = 4 * TFN * K;
        std::vector<float> surfData(surfSize);
        int off = 0;
        for (int k = 0; k < K; ++k)
            for (int f = 0; f < TFN; ++f)
                surfData[off++] = static_cast<float>(mesh.nx(f, k));
        for (int k = 0; k < K; ++k)
            for (int f = 0; f < TFN; ++f)
                surfData[off++] = static_cast<float>(mesh.ny(f, k));
        for (int k = 0; k < K; ++k)
            for (int f = 0; f < TFN; ++f)
                surfData[off++] = static_cast<float>(mesh.Fscale(f, k));
        for (int k = 0; k < K; ++k)
            for (int f = 0; f < TFN; ++f)
                surfData[off++] = static_cast<float>(mesh.boundaryImpedance(k * TFN + f));

        s2d_.surfBuf = createBuffer(GL_STATIC_DRAW, surfSize * sizeof(float), surfData.data());
    }

    // Connectivity: vmapM(TFN*K), vmapP(TFN*K), EToE(K*Nfaces), EToF(K*Nfaces)
    {
        int connSize = 2 * TFN * K + 2 * K * Nfaces;
        std::vector<int> connData(connSize);
        int off = 0;
        for (int i = 0; i < TFN * K; ++i)
            connData[off++] = mesh.vmapM(i);
        for (int i = 0; i < TFN * K; ++i)
            connData[off++] = mesh.vmapP(i);
        for (int k = 0; k < K; ++k)
            for (int f = 0; f < Nfaces; ++f)
                connData[off++] = mesh.EToE(k, f);
        for (int k = 0; k < K; ++k)
            for (int f = 0; f < Nfaces; ++f)
                connData[off++] = mesh.EToF(k, f);

        s2d_.connBuf = createBuffer(GL_STATIC_DRAW, connSize * sizeof(int), connData.data());
    }

    // Source/listener buffer: sourceWeights(Np), sourceElem(1), sourcePulse(1),
    //                         listenerWeights(Np), result(1) = 2*Np+3 floats
    s2d_.srcBuf = createBuffer(GL_DYNAMIC_DRAW, (2 * Np + 3) * sizeof(float));

    qInfo() << "DG GPU 2D: initialized" << K << "elements, Np=" << Np
            << ", local_size=" << s2d_.localSize
            << ", field memory:" << (3 * Np * K * sizeof(float) / 1024) << "KB";

    return true;
}

void DGGpuCompute::resetFields2D() {
    int Np = s2d_.Np;
    int K = s2d_.K;
    int sz = 3 * Np * K * sizeof(float);
    std::vector<float> zeros(3 * Np * K, 0.0f);
    uploadBuffer(s2d_.fieldBuf, sz, zeros.data());
    uploadBuffer(s2d_.rhsBuf, sz, zeros.data());
    uploadBuffer(s2d_.resBuf, sz, zeros.data());
}

void DGGpuCompute::setSource2D(int elem, const VecXd& weights) {
    s2d_.sourceElem = elem;
    int Np = s2d_.Np;
    std::vector<float> data(2 * Np + 3, 0.0f);
    for (int i = 0; i < Np; ++i)
        data[i] = static_cast<float>(weights(i));
    data[Np] = static_cast<float>(elem);
    data[Np + 1] = 0.0f; // pulse (updated per call)
    uploadBuffer(s2d_.srcBuf, data.size() * sizeof(float), data.data());
}

void DGGpuCompute::setListener2D(int elem, const VecXd& weights) {
    s2d_.listenerElem = elem;
    int Np = s2d_.Np;
    // Update listener weights at offset (Np+2) in srcBuf
    std::vector<float> wt(Np);
    for (int i = 0; i < Np; ++i)
        wt[i] = static_cast<float>(weights(i));
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, s2d_.srcBuf);
    gl_->glBufferSubData(GL_SHADER_STORAGE_BUFFER, (Np + 2) * sizeof(float),
                         Np * sizeof(float), wt.data());
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void DGGpuCompute::computeRHS2D(float sourcePulse) {
    int Np = s2d_.Np;
    int K = s2d_.K;

    // Update source pulse in srcBuf
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, s2d_.srcBuf);
    gl_->glBufferSubData(GL_SHADER_STORAGE_BUFFER, (Np + 1) * sizeof(float),
                         sizeof(float), &sourcePulse);
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    gl_->glUseProgram(s2d_.rhsProgram);

    // Set uniforms
    gl_->glUniform1i(gl_->glGetUniformLocation(s2d_.rhsProgram, "uK"), K);
    gl_->glUniform1f(gl_->glGetUniformLocation(s2d_.rhsProgram, "uRho"),
                     static_cast<float>(AIR_DENSITY));
    gl_->glUniform1f(gl_->glGetUniformLocation(s2d_.rhsProgram, "uC"),
                     static_cast<float>(SOUND_SPEED));

    // Bind SSBOs
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s2d_.fieldBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s2d_.rhsBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, s2d_.opBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, s2d_.geoBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, s2d_.surfBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, s2d_.connBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, s2d_.srcBuf);

    gl_->glDispatchCompute(K, 1, 1);
    gl_->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void DGGpuCompute::rkStageUpdate2D(float rk_a, float rk_b, float dt) {
    int totalNodes = 3 * s2d_.Np * s2d_.K;

    gl_->glUseProgram(rkProgram_);

    gl_->glUniform1f(gl_->glGetUniformLocation(rkProgram_, "uRkA"), rk_a);
    gl_->glUniform1f(gl_->glGetUniformLocation(rkProgram_, "uRkB"), rk_b);
    gl_->glUniform1f(gl_->glGetUniformLocation(rkProgram_, "uDt"), dt);
    gl_->glUniform1i(gl_->glGetUniformLocation(rkProgram_, "uTotalNodes"), totalNodes);

    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s2d_.fieldBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s2d_.rhsBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s2d_.resBuf);

    int groups = (totalNodes + 255) / 256;
    gl_->glDispatchCompute(groups, 1, 1);
    gl_->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

float DGGpuCompute::readListenerPressure2D() {
    int Np = s2d_.Np;

    gl_->glUseProgram(readbackProgram_);

    gl_->glUniform1i(gl_->glGetUniformLocation(readbackProgram_, "uListenerElem"),
                     s2d_.listenerElem);
    gl_->glUniform1i(gl_->glGetUniformLocation(readbackProgram_, "uNp"), Np);
    gl_->glUniform1i(gl_->glGetUniformLocation(readbackProgram_, "uK"), s2d_.K);

    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s2d_.fieldBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, s2d_.srcBuf);

    gl_->glDispatchCompute(1, 1, 1);
    gl_->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

    // Read result from srcBuf at offset (2*Np+2)
    float result = 0.0f;
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, s2d_.srcBuf);
    gl_->glGetBufferSubData(GL_SHADER_STORAGE_BUFFER,
                            (2 * Np + 2) * sizeof(float),
                            sizeof(float), &result);
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    return result;
}

// ==================== 3D SHADER & INIT ====================

std::string DGGpuCompute::generateRHSShader3D(int Np, int totalFaceNodes, int localSize) {
    int Nfp = totalFaceNodes / 4;
    std::ostringstream ss;
    ss << R"(#version 430
layout(local_size_x = )" << localSize << R"() in;

// SSBO 0: fields (p, ux, uy, uz) [4 * Np * K]
layout(std430, binding = 0) readonly buffer Fields { float fields[]; };
// SSBO 1: RHS [4 * Np * K]
layout(std430, binding = 1) buffer RHS { float rhs[]; };
// SSBO 3: operators (Dr, Ds, Dt, LIFT)
layout(std430, binding = 3) readonly buffer Ops { float ops[]; };
// SSBO 4: geometry (rx,ry,rz, sx,sy,sz, tx,ty,tz) [9 * Np * K]
layout(std430, binding = 4) readonly buffer Geo { float geo[]; };
// SSBO 5: surface (nx,ny,nz, Fscale, boundaryImpedance) [5 * TFN * K]
layout(std430, binding = 5) readonly buffer Surf { float surf[]; };
// SSBO 6: connectivity
layout(std430, binding = 6) readonly buffer Conn { int conn[]; };
// SSBO 7: source data
layout(std430, binding = 7) readonly buffer Src { float srcData[]; };

const int cNp = )" << Np << R"(;
const int cNfaces = 4;
const int cNfp = )" << Nfp << R"(;
const int cTFN = )" << totalFaceNodes << R"(;

uniform int uK;
uniform float uRho;
uniform float uC;

shared float s_p[)" << localSize << R"(];
shared float s_ux[)" << localSize << R"(];
shared float s_uy[)" << localSize << R"(];
shared float s_uz[)" << localSize << R"(];
shared float s_flux_p[)" << localSize << R"(];
shared float s_flux_ux[)" << localSize << R"(];
shared float s_flux_uy[)" << localSize << R"(];
shared float s_flux_uz[)" << localSize << R"(];

void main() {
    int k = int(gl_WorkGroupID.x);
    int tid = int(gl_LocalInvocationID.x);

    if (k >= uK) return;

    float c2 = uC * uC;
    float invRho = 1.0 / uRho;
    float Z = uRho * uC;
    float invZ = 1.0 / Z;

    int NK = uK * cNp;
    int fieldBase = k * cNp;

    if (tid < cNp) {
        s_p[tid]  = fields[fieldBase + tid];
        s_ux[tid] = fields[NK + fieldBase + tid];
        s_uy[tid] = fields[2*NK + fieldBase + tid];
        s_uz[tid] = fields[3*NK + fieldBase + tid];
    }
    barrier();

    // Volume terms: 3 reference derivatives per field
    if (tid < cNp) {
        float dpdr = 0.0, dpds = 0.0, dpdt = 0.0;
        float duxdr = 0.0, duxds = 0.0, duxdt = 0.0;
        float duydr = 0.0, duyds = 0.0, duydt = 0.0;
        float duzdr = 0.0, duzds = 0.0, duzdt = 0.0;

        int drBase = 0;
        int dsBase = cNp * cNp;
        int dtBase = 2 * cNp * cNp;

        for (int j = 0; j < cNp; ++j) {
            float dr_v = ops[drBase + tid * cNp + j];
            float ds_v = ops[dsBase + tid * cNp + j];
            float dt_v = ops[dtBase + tid * cNp + j];

            dpdr += dr_v * s_p[j]; dpds += ds_v * s_p[j]; dpdt += dt_v * s_p[j];
            duxdr += dr_v * s_ux[j]; duxds += ds_v * s_ux[j]; duxdt += dt_v * s_ux[j];
            duydr += dr_v * s_uy[j]; duyds += ds_v * s_uy[j]; duydt += dt_v * s_uy[j];
            duzdr += dr_v * s_uz[j]; duzds += ds_v * s_uz[j]; duzdt += dt_v * s_uz[j];
        }

        int geoBase = k * cNp + tid;
        float rxv = geo[geoBase],        ryv = geo[NK + geoBase],   rzv = geo[2*NK + geoBase];
        float sxv = geo[3*NK + geoBase], syv = geo[4*NK + geoBase], szv = geo[5*NK + geoBase];
        float txv = geo[6*NK + geoBase], tyv = geo[7*NK + geoBase], tzv = geo[8*NK + geoBase];

        float dpdx = rxv*dpdr + sxv*dpds + txv*dpdt;
        float dpdy = ryv*dpdr + syv*dpds + tyv*dpdt;
        float dpdz = rzv*dpdr + szv*dpds + tzv*dpdt;

        float duxdx = rxv*duxdr + sxv*duxds + txv*duxdt;
        float duydy = ryv*duydr + syv*duyds + tyv*duydt;
        float duzdz = rzv*duzdr + szv*duzds + tzv*duzdt;
        float divU = duxdx + duydy + duzdz;

        int outIdx = fieldBase + tid;
        rhs[outIdx]        = -uRho * c2 * divU;
        rhs[NK + outIdx]   = -invRho * dpdx;
        rhs[2*NK + outIdx] = -invRho * dpdy;
        rhs[3*NK + outIdx] = -invRho * dpdz;
    }
    barrier();

    // Surface flux
    int TK = uK * cTFN;
    if (tid < cTFN) {
        int fid = tid;
        int f = fid / cNfp;
        int surfIdx = k * cTFN + fid;

        int connBase_vmapM = 0;
        int connBase_vmapP = TK;
        int connBase_EToE  = 2 * TK;
        int connBase_EToF  = 2 * TK + uK * cNfaces;

        int EToE_kf = conn[connBase_EToE + k * cNfaces + f];
        int EToF_kf = conn[connBase_EToF + k * cNfaces + f];
        bool isBoundary = (EToE_kf == k && EToF_kf == f);

        int idM = conn[connBase_vmapM + surfIdx];
        int idP = conn[connBase_vmapP + surfIdx];

        int kM = idM / cNp, nM = idM - kM * cNp;
        float pM  = fields[kM * cNp + nM];
        float uxM = fields[NK + kM * cNp + nM];
        float uyM = fields[2*NK + kM * cNp + nM];
        float uzM = fields[3*NK + kM * cNp + nM];

        float pP, uxP, uyP, uzP;
        int sd = k * cTFN + fid;
        float fnx = surf[sd];
        float fny = surf[TK + sd];
        float fnz = surf[2*TK + sd];

        if (isBoundary) {
            float Zwall = surf[4*TK + surfIdx];
            if (Zwall < 1e-10) Zwall = 1e10;
            float R = (Zwall - Z) / (Zwall + Z);
            float unM = uxM*fnx + uyM*fny + uzM*fnz;
            float unP = -R * unM;
            pP = R * pM;
            float utx = uxM - unM*fnx;
            float uty = uyM - unM*fny;
            float utz = uzM - unM*fnz;
            uxP = unP*fnx + utx;
            uyP = unP*fny + uty;
            uzP = unP*fnz + utz;
        } else {
            int kP = idP / cNp, nP = idP - kP * cNp;
            pP  = fields[kP * cNp + nP];
            uxP = fields[NK + kP * cNp + nP];
            uyP = fields[2*NK + kP * cNp + nP];
            uzP = fields[3*NK + kP * cNp + nP];
        }

        float dp = pP - pM;
        float dunM = (uxP-uxM)*fnx + (uyP-uyM)*fny + (uzP-uzM)*fnz;
        float fsc = surf[3*TK + sd];

        s_flux_p[tid]  = 0.5 * (uC * invZ * dp - c2 * dunM) * fsc;
        float fluxUn   = 0.5 * (-invRho * dp + uC * Z * invRho * dunM) * fsc;
        s_flux_ux[tid] = fluxUn * fnx;
        s_flux_uy[tid] = fluxUn * fny;
        s_flux_uz[tid] = fluxUn * fnz;
    }
    barrier();

    // LIFT + source
    if (tid < cNp) {
        float lP = 0.0, lUx = 0.0, lUy = 0.0, lUz = 0.0;
        int liftBase = 3 * cNp * cNp;  // after Dr, Ds, Dt

        for (int j = 0; j < cTFN; ++j) {
            float l = ops[liftBase + tid * cTFN + j];
            lP  += l * s_flux_p[j];
            lUx += l * s_flux_ux[j];
            lUy += l * s_flux_uy[j];
            lUz += l * s_flux_uz[j];
        }

        int outIdx = fieldBase + tid;
        rhs[outIdx]        += lP;
        rhs[NK + outIdx]   += lUx;
        rhs[2*NK + outIdx] += lUy;
        rhs[3*NK + outIdx] += lUz;

        int srcElem = int(srcData[cNp]);
        if (k == srcElem) {
            float pulse = srcData[cNp + 1];
            rhs[outIdx] += pulse * srcData[tid];
        }
    }
}
)";
    return ss.str();
}

bool DGGpuCompute::init3D(const Basis3D& basis, const Mesh3D& mesh) {
    if (!available_) return false;

    s3d_.Np = basis.Np;
    s3d_.K = mesh.K;
    s3d_.Nfp = basis.Nfp;
    s3d_.Nfaces = basis.Nfaces;
    s3d_.totalFaceNodes = basis.Nfp * basis.Nfaces;
    s3d_.localSize = nextPow2(std::max(basis.Np, s3d_.totalFaceNodes));

    std::string rhsSrc = generateRHSShader3D(s3d_.Np, s3d_.totalFaceNodes, s3d_.localSize);
    if (!compileProgram(s3d_.rhsProgram, rhsSrc)) {
        qWarning() << "DG GPU 3D: RHS shader failed:" << QString::fromStdString(lastError_);
        return false;
    }
    if (!rkProgram_ && !compileProgram(rkProgram_, generateRKShader())) return false;
    if (!readbackProgram_ && !compileProgram(readbackProgram_, generateReadbackShader())) return false;

    int Np = s3d_.Np;
    int K = s3d_.K;
    int TFN = s3d_.totalFaceNodes;
    int Nfaces = s3d_.Nfaces;

    // Fields: 4 fields for 3D (p, ux, uy, uz)
    s3d_.fieldBuf = createBuffer(GL_DYNAMIC_DRAW, 4 * Np * K * sizeof(float));
    s3d_.rhsBuf   = createBuffer(GL_DYNAMIC_DRAW, 4 * Np * K * sizeof(float));
    s3d_.resBuf   = createBuffer(GL_DYNAMIC_DRAW, 4 * Np * K * sizeof(float));

    // Operators: Dr + Ds + Dt + LIFT
    {
        int opSize = 3 * Np * Np + Np * TFN;
        std::vector<float> opData(opSize);
        int off = 0;
        for (int i = 0; i < Np; ++i)
            for (int j = 0; j < Np; ++j)
                opData[off++] = static_cast<float>(basis.Dr(i, j));
        for (int i = 0; i < Np; ++i)
            for (int j = 0; j < Np; ++j)
                opData[off++] = static_cast<float>(basis.Ds(i, j));
        for (int i = 0; i < Np; ++i)
            for (int j = 0; j < Np; ++j)
                opData[off++] = static_cast<float>(basis.Dt(i, j));
        for (int i = 0; i < Np; ++i)
            for (int j = 0; j < TFN; ++j)
                opData[off++] = static_cast<float>(basis.LIFT(i, j));
        s3d_.opBuf = createBuffer(GL_STATIC_DRAW, opSize * sizeof(float), opData.data());
    }

    // Geo: 9 components (rx,ry,rz, sx,sy,sz, tx,ty,tz)
    {
        int geoSize = 9 * Np * K;
        std::vector<float> geoData(geoSize);
        int off = 0;
        auto pack = [&](const MatXd& m) {
            for (int kk = 0; kk < K; ++kk)
                for (int i = 0; i < Np; ++i)
                    geoData[off++] = static_cast<float>(m(i, kk));
        };
        pack(mesh.rx); pack(mesh.ry); pack(mesh.rz);
        pack(mesh.sx); pack(mesh.sy); pack(mesh.sz);
        pack(mesh.tx); pack(mesh.ty); pack(mesh.tz);
        s3d_.geoBuf = createBuffer(GL_STATIC_DRAW, geoSize * sizeof(float), geoData.data());
    }

    // Surface: nx,ny,nz, Fscale, boundaryImpedance (5 * TFN * K)
    {
        int surfSize = 5 * TFN * K;
        std::vector<float> surfData(surfSize);
        int off = 0;
        for (int kk = 0; kk < K; ++kk) for (int f = 0; f < TFN; ++f) surfData[off++] = static_cast<float>(mesh.nx(f, kk));
        for (int kk = 0; kk < K; ++kk) for (int f = 0; f < TFN; ++f) surfData[off++] = static_cast<float>(mesh.ny(f, kk));
        for (int kk = 0; kk < K; ++kk) for (int f = 0; f < TFN; ++f) surfData[off++] = static_cast<float>(mesh.nz(f, kk));
        for (int kk = 0; kk < K; ++kk) for (int f = 0; f < TFN; ++f) surfData[off++] = static_cast<float>(mesh.Fscale(f, kk));
        for (int kk = 0; kk < K; ++kk) for (int f = 0; f < TFN; ++f) surfData[off++] = static_cast<float>(mesh.boundaryImpedance(kk * TFN + f));
        s3d_.surfBuf = createBuffer(GL_STATIC_DRAW, surfSize * sizeof(float), surfData.data());
    }

    // Connectivity
    {
        int connSize = 2 * TFN * K + 2 * K * Nfaces;
        std::vector<int> connData(connSize);
        int off = 0;
        for (int i = 0; i < TFN * K; ++i) connData[off++] = mesh.vmapM(i);
        for (int i = 0; i < TFN * K; ++i) connData[off++] = mesh.vmapP(i);
        for (int kk = 0; kk < K; ++kk) for (int f = 0; f < Nfaces; ++f) connData[off++] = mesh.EToE(kk, f);
        for (int kk = 0; kk < K; ++kk) for (int f = 0; f < Nfaces; ++f) connData[off++] = mesh.EToF(kk, f);
        s3d_.connBuf = createBuffer(GL_STATIC_DRAW, connSize * sizeof(int), connData.data());
    }

    s3d_.srcBuf = createBuffer(GL_DYNAMIC_DRAW, (2 * Np + 3) * sizeof(float));

    qInfo() << "DG GPU 3D: initialized" << K << "elements, Np=" << Np
            << ", local_size=" << s3d_.localSize;
    return true;
}

void DGGpuCompute::resetFields3D() {
    int sz = 4 * s3d_.Np * s3d_.K * sizeof(float);
    std::vector<float> zeros(4 * s3d_.Np * s3d_.K, 0.0f);
    uploadBuffer(s3d_.fieldBuf, sz, zeros.data());
    uploadBuffer(s3d_.rhsBuf, sz, zeros.data());
    uploadBuffer(s3d_.resBuf, sz, zeros.data());
}

void DGGpuCompute::setSource3D(int elem, const VecXd& weights) {
    s3d_.sourceElem = elem;
    int Np = s3d_.Np;
    std::vector<float> data(2 * Np + 3, 0.0f);
    for (int i = 0; i < Np; ++i)
        data[i] = static_cast<float>(weights(i));
    data[Np] = static_cast<float>(elem);
    uploadBuffer(s3d_.srcBuf, data.size() * sizeof(float), data.data());
}

void DGGpuCompute::setListener3D(int elem, const VecXd& weights) {
    s3d_.listenerElem = elem;
    int Np = s3d_.Np;
    std::vector<float> wt(Np);
    for (int i = 0; i < Np; ++i) wt[i] = static_cast<float>(weights(i));
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, s3d_.srcBuf);
    gl_->glBufferSubData(GL_SHADER_STORAGE_BUFFER, (Np + 2) * sizeof(float),
                         Np * sizeof(float), wt.data());
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void DGGpuCompute::computeRHS3D(float sourcePulse) {
    int Np = s3d_.Np;
    int K = s3d_.K;

    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, s3d_.srcBuf);
    gl_->glBufferSubData(GL_SHADER_STORAGE_BUFFER, (Np + 1) * sizeof(float),
                         sizeof(float), &sourcePulse);
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    gl_->glUseProgram(s3d_.rhsProgram);
    gl_->glUniform1i(gl_->glGetUniformLocation(s3d_.rhsProgram, "uK"), K);
    gl_->glUniform1f(gl_->glGetUniformLocation(s3d_.rhsProgram, "uRho"), static_cast<float>(AIR_DENSITY));
    gl_->glUniform1f(gl_->glGetUniformLocation(s3d_.rhsProgram, "uC"), static_cast<float>(SOUND_SPEED));

    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s3d_.fieldBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s3d_.rhsBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, s3d_.opBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, s3d_.geoBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, s3d_.surfBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, s3d_.connBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, s3d_.srcBuf);

    gl_->glDispatchCompute(K, 1, 1);
    gl_->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void DGGpuCompute::rkStageUpdate3D(float rk_a, float rk_b, float dt) {
    int totalNodes = 4 * s3d_.Np * s3d_.K;

    gl_->glUseProgram(rkProgram_);
    gl_->glUniform1f(gl_->glGetUniformLocation(rkProgram_, "uRkA"), rk_a);
    gl_->glUniform1f(gl_->glGetUniformLocation(rkProgram_, "uRkB"), rk_b);
    gl_->glUniform1f(gl_->glGetUniformLocation(rkProgram_, "uDt"), dt);
    gl_->glUniform1i(gl_->glGetUniformLocation(rkProgram_, "uTotalNodes"), totalNodes);

    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s3d_.fieldBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s3d_.rhsBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s3d_.resBuf);

    int groups = (totalNodes + 255) / 256;
    gl_->glDispatchCompute(groups, 1, 1);
    gl_->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

float DGGpuCompute::readListenerPressure3D() {
    int Np = s3d_.Np;

    gl_->glUseProgram(readbackProgram_);
    gl_->glUniform1i(gl_->glGetUniformLocation(readbackProgram_, "uListenerElem"), s3d_.listenerElem);
    gl_->glUniform1i(gl_->glGetUniformLocation(readbackProgram_, "uNp"), Np);
    gl_->glUniform1i(gl_->glGetUniformLocation(readbackProgram_, "uK"), s3d_.K);

    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s3d_.fieldBuf);
    gl_->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, s3d_.srcBuf);

    gl_->glDispatchCompute(1, 1, 1);
    gl_->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

    float result = 0.0f;
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, s3d_.srcBuf);
    gl_->glGetBufferSubData(GL_SHADER_STORAGE_BUFFER,
                            (2 * Np + 2) * sizeof(float),
                            sizeof(float), &result);
    gl_->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return result;
}

} // namespace dg
} // namespace prs
