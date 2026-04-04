#include "Wall.h"

#include <cmath>

namespace prs {

Vec3f Wall::reflectPoint(const Vec3f& point) const {
    Vec3f n = normal();
    Vec3f v0 = triangle.v0;
    float d = n.dot(point - v0);
    return point - 2.0f * d * n;
}

bool Wall::isPointOnSameSide(const Vec3f& point) const {
    Vec3f n = normal();
    float d = n.dot(point - triangle.v0);
    return d > 0.0f;
}

float Wall::area() const {
    Vec3f e1 = triangle.v1 - triangle.v0;
    Vec3f e2 = triangle.v2 - triangle.v0;
    return 0.5f * e1.cross(e2).norm();
}

} // namespace prs
