#pragma once

#include "core/Types.h"

namespace prs {

struct Wall {
    Triangle triangle;
    float energyAbsorption = 0.2f;
    float scattering       = 0.1f;

    Vec3f normal() const { return triangle.normal.normalized(); }
    Vec3f centroid() const {
        return (triangle.v0 + triangle.v1 + triangle.v2) / 3.0f;
    }

    Vec3f reflectPoint(const Vec3f& point) const;
    bool isPointOnSameSide(const Vec3f& point) const;
    float area() const;
};

} // namespace prs
