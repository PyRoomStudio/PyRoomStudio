#pragma once

#include "core/Material.h"
#include "core/Types.h"

#include <array>

namespace prs {

struct Wall {
    Triangle triangle;
    std::array<float, NUM_FREQ_BANDS> absorption = {0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f};
    float scattering = 0.1f;

    Vec3f normal() const { return triangle.normal.normalized(); }
    Vec3f centroid() const { return (triangle.v0 + triangle.v1 + triangle.v2) / 3.0f; }

    float averageAbsorption() const {
        float sum = 0.0f;
        for (float a : absorption)
            sum += a;
        return sum / NUM_FREQ_BANDS;
    }

    Vec3f reflectPoint(const Vec3f& point) const;
    bool isPointOnSameSide(const Vec3f& point) const;
    float area() const;
};

struct AcousticSurface {
    Vec3f normal;
    Vec3f centroid;
    Vec3f planePoint;
    float area = 0.0f;
    std::array<float, NUM_FREQ_BANDS> absorption = {0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f};
    float scattering = 0.1f;

    float averageAbsorption() const {
        float sum = 0.0f;
        for (float a : absorption)
            sum += a;
        return sum / NUM_FREQ_BANDS;
    }

    Vec3f reflectPoint(const Vec3f& point) const {
        float d = normal.dot(point - planePoint);
        return point - 2.0f * d * normal;
    }
};

} // namespace prs
