// camera.hpp -- orbit ("turntable") camera.
//
//   left drag    orbit
//   right drag   pan
//   wheel        dolly
//
// Pitch is clamped just short of the poles so the up-vector never degenerates.
#pragma once

#include <algorithm>
#include <cmath>

#include "linalg.hpp"

namespace hc {

class OrbitCamera {
public:
    void frame(double radius) {
        target_ = Vec3{0.0, 0.0, 0.0};
        distance_ = std::max(radius * 2.6, 1e-3);
        minDistance_ = radius * 0.05;
        maxDistance_ = radius * 60.0;
        yaw_ = -0.9;
        pitch_ = 0.42;
    }

    void orbit(double dxPixels, double dyPixels) {
        yaw_   += dxPixels * 0.009;
        pitch_ += dyPixels * 0.009;
        const double lim = 1.55334;  // ~89 degrees
        pitch_ = std::clamp(pitch_, -lim, lim);
    }

    void pan(double dxPixels, double dyPixels, int viewportH) {
        // Scale so a pixel of drag moves roughly a pixel of scene at the target.
        const double scale = 2.0 * distance_ * std::tan(0.5 * fovY_) / std::max(viewportH, 1);
        const Vec3 fwd   = forward();
        const Vec3 right = normalized(cross(fwd, Vec3{0, 0, 1}));
        const Vec3 up    = cross(right, fwd);
        target_ += right * (-dxPixels * scale);
        target_ += up * (dyPixels * scale);
    }

    void dolly(double wheelTicks) {
        distance_ *= std::pow(0.88, wheelTicks);
        distance_ = std::clamp(distance_, minDistance_, maxDistance_);
    }

    Vec3 eye() const { return target_ - forward() * distance_; }

    Mat4 viewProj(int w, int h) const {
        const float aspect = (h > 0) ? (float)w / (float)h : 1.0f;
        // Near/far track the orbit distance so precision stays usable across
        // the whole 0.2 m .. 4 m coil range.
        const float zNear = (float)std::max(distance_ * 0.002, 1e-4);
        const float zFar  = (float)(distance_ * 50.0 + 10.0);
        return perspective((float)fovY_, aspect, zNear, zFar) *
               lookAt(eye(), target_, Vec3{0, 0, 1});
    }

    double distance() const { return distance_; }

private:
    // z-up spherical coordinates: the eye sits at
    //   target + distance * (cos p cos y, cos p sin y, sin p)
    // so the view direction is the negative of that unit vector.
    Vec3 forward() const {
        const double cp = std::cos(pitch_);
        return Vec3{-cp * std::cos(yaw_), -cp * std::sin(yaw_), -std::sin(pitch_)};
    }

    double yaw_ = -0.9;      // rad, about +z
    double pitch_ = 0.42;    // rad, above the xy-plane
    double distance_ = 3.0;  // m
    double minDistance_ = 0.05, maxDistance_ = 200.0;
    double fovY_ = 0.9;      // rad (~52 deg)
    Vec3 target_{0, 0, 0};
};

}  // namespace hc
