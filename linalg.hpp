// linalg.hpp -- minimal vector / matrix math.
//
// Deliberately dependency-free: the whole project needs perspective, lookAt,
// a 4x4 multiply and 3-vectors, which is not worth pulling GLM in for.  Vec3
// is double precision; Mat4 is float because it is handed straight to OpenGL.
#pragma once

#include <cmath>

namespace hc {

// ---------------------------------------------------------------- Vec3 -----

struct Vec3 {
    double x = 0.0, y = 0.0, z = 0.0;

    Vec3() = default;
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(const Vec3& a, double s)      { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator*(double s, const Vec3& a)      { return a * s; }
inline Vec3 operator/(const Vec3& a, double s)      { return {a.x / s, a.y / s, a.z / s}; }
inline Vec3& operator+=(Vec3& a, const Vec3& b)     { a.x += b.x; a.y += b.y; a.z += b.z; return a; }

inline double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

inline double norm(const Vec3& a)  { return std::sqrt(dot(a, a)); }
inline double norm2(const Vec3& a) { return dot(a, a); }

inline Vec3 normalized(const Vec3& a) {
    const double n = norm(a);
    return (n > 0.0) ? a / n : Vec3{0.0, 0.0, 0.0};
}

// ---------------------------------------------------------------- Mat4 -----
// Column-major, OpenGL convention: m[col * 4 + row].

struct Mat4 {
    float m[16] = {1, 0, 0, 0,
                   0, 1, 0, 0,
                   0, 0, 1, 0,
                   0, 0, 0, 1};
};

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int c = 0; c < 4; ++c) {
        for (int row = 0; row < 4; ++row) {
            float s = 0.0f;
            for (int k = 0; k < 4; ++k) s += a.m[k * 4 + row] * b.m[c * 4 + k];
            r.m[c * 4 + row] = s;
        }
    }
    return r;
}

inline Mat4 perspective(float fovyRad, float aspect, float zNear, float zFar) {
    const float f = 1.0f / std::tan(fovyRad * 0.5f);
    Mat4 r;
    for (float& v : r.m) v = 0.0f;
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (zFar + zNear) / (zNear - zFar);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    return r;
}

inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    const Vec3 f = normalized(center - eye);
    const Vec3 s = normalized(cross(f, up));
    const Vec3 u = cross(s, f);

    Mat4 r;
    r.m[0] = (float)s.x;  r.m[4] = (float)s.y;  r.m[8]  = (float)s.z;  r.m[12] = (float)-dot(s, eye);
    r.m[1] = (float)u.x;  r.m[5] = (float)u.y;  r.m[9]  = (float)u.z;  r.m[13] = (float)-dot(u, eye);
    r.m[2] = (float)-f.x; r.m[6] = (float)-f.y; r.m[10] = (float)-f.z; r.m[14] = (float)dot(f, eye);
    r.m[3] = 0.0f;        r.m[7] = 0.0f;        r.m[11] = 0.0f;        r.m[15] = 1.0f;
    return r;
}

}  // namespace hc
