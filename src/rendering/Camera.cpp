#include <cmath>
#include <algorithm>

#include "Camera.h"

#include <QOpenGLFunctions>
#include <QtGui/QOpenGLContext>

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

namespace prs {

Camera::Camera() = default;

void Camera::orbit(float dx, float dy) {
    heading_ -= dx * 0.5f;
    pitch_   += dy * 0.5f;
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
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
    return Vec3f(x, y, z);
}

void Camera::applyViewMatrix() const {
    Vec3f eye = eyePosition();
    glLoadIdentity();
    gluLookAt(eye.x(), eye.y(), eye.z(),
              0.0, 0.0, 0.0,
              0.0, 0.0, 1.0);
}

} // namespace prs
