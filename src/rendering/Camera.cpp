#include <cmath>
#include <algorithm>

#include "Camera.h"
#include "GLHeaders.h"

#include <QOpenGLFunctions>
#include <QtGui/QOpenGLContext>

namespace prs {

Camera::Camera() = default;

void Camera::orbit(float dx, float dy) {
    heading_ -= dx * 0.5f;
    pitch_   += dy * 0.5f;
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
}

void Camera::pan(float right, float forward) {
    float hr = heading_ * static_cast<float>(M_PI) / 180.0f;
    Vec3f rightDir(std::cos(hr), std::sin(hr), 0.0f);
    Vec3f forwardDir(-std::sin(hr), std::cos(hr), 0.0f); // eye -> target, flattened
    target_ += rightDir * right + forwardDir * forward;
}

void Camera::zoom(float delta, float modelSize) {
    float step = 0.1f * modelSize;
    distance_ -= delta * step;
    distance_ = std::clamp(distance_, minDist_, maxDist_);
}

void Camera::reset(float modelSize) {
    heading_  = 35.0f;
    pitch_    = 35.0f;
    distance_ = 2.5f * modelSize;
    target_   = Vec3f::Zero();
    minDist_  = 0.2f * modelSize;
    maxDist_  = 5.0f * modelSize;
}

void Camera::setDistanceLimits(float minDist, float maxDist) {
    minDist_ = minDist;
    maxDist_ = maxDist;
}

Vec3f Camera::eyePosition() const {
    float hr = heading_ * static_cast<float>(M_PI) / 180.0f;
    float pr = pitch_   * static_cast<float>(M_PI) / 180.0f;
    float x =  distance_ * std::sin(hr) * std::cos(pr);
    float y = -distance_ * std::cos(hr) * std::cos(pr);
    float z =  distance_ * std::sin(pr);
    return target_ + Vec3f(x, y, z);
}

void Camera::applyViewMatrix() const {
    Vec3f eye = eyePosition();
    glLoadIdentity();
    gluLookAt(eye.x(), eye.y(), eye.z(),
              target_.x(), target_.y(), target_.z(),
              0.0, 0.0, 1.0);
}

} // namespace prs
