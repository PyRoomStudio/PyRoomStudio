#pragma once

#include "core/Types.h"

namespace prs {

class Camera {
public:
    Camera();

    void orbit(float dx, float dy);
    // Moves the look-at target across the ground plane (z-up), relative to the heading.
    void pan(float right, float forward);
    void zoom(float delta, float modelSize);
    void reset(float modelSize);

    void setDistanceLimits(float minDist, float maxDist);

    float heading()  const { return heading_; }
    float pitch()    const { return pitch_; }
    float distance() const { return distance_; }

    void  setDistance(float d) { distance_ = d; }
    float minDistance() const { return minDist_; }
    float maxDistance() const { return maxDist_; }

    Vec3f target() const { return target_; }
    void  setTarget(const Vec3f& t) { target_ = t; }

    Vec3f eyePosition() const;

    void applyViewMatrix() const;

private:
    float heading_  = 35.0f;
    float pitch_    = 35.0f;
    float distance_ = 5.0f;
    float minDist_  = 0.5f;
    float maxDist_  = 50.0f;
    Vec3f target_   = Vec3f::Zero();
};

} // namespace prs
