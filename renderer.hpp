// renderer.hpp -- a deliberately small immediate-mode batcher.
//
// The whole scene is a few thousand vertices that change only when a slider
// moves, so there is no reason for anything cleverer than "clear the vectors,
// refill them, upload, draw".  Geometry is sorted into three streams: opaque
// triangles, lines, and translucent triangles, drawn in that order so the
// glassy stable-region shell never occludes what sits inside it.
#pragma once

#include <cmath>
#include <string>
#include <vector>

#include "gl_core.hpp"
#include "linalg.hpp"

namespace hc {

struct Color {
    float r = 1, g = 1, b = 1, a = 1;
};

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

class Batch {
public:
    void clear() {
        lines_.clear();
        opaque_.clear();
        blended_.clear();
    }

    void line(const Vec3& a, const Vec3& b, const Color& c) {
        push(lines_, a, c);
        push(lines_, b, c);
    }

    // `blend` routes the triangle into the translucent pass, which is drawn
    // after everything else with depth writes disabled.
    void tri(const Vec3& a, const Vec3& b, const Vec3& c, const Color& col, bool blend = false) {
        auto& dst = blend ? blended_ : opaque_;
        push(dst, a, col);
        push(dst, b, col);
        push(dst, c, col);
    }

    void quad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Color& col,
              bool blend = false) {
        tri(a, b, c, col, blend);
        tri(a, c, d, col, blend);
    }

    // A square-section bar from a to b -- used to give the coil conductors
    // real thickness, since glLineWidth > 1 is not dependable in core profile.
    void bar(const Vec3& a, const Vec3& b, double t, const Color& c) {
        const Vec3 axis = normalized(b - a);
        Vec3 ref = (std::fabs(axis.z) < 0.9) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        const Vec3 u = normalized(cross(axis, ref)) * (0.5 * t);
        const Vec3 v = normalized(cross(axis, u)) * (0.5 * t);

        const Vec3 q[8] = {a - u - v, a + u - v, a + u + v, a - u + v,
                           b - u - v, b + u - v, b + u + v, b - u + v};
        quad(q[0], q[1], q[2], q[3], c);
        quad(q[7], q[6], q[5], q[4], c);
        quad(q[0], q[1], q[5], q[4], c);
        quad(q[1], q[2], q[6], q[5], c);
        quad(q[2], q[3], q[7], q[6], c);
        quad(q[3], q[0], q[4], q[7], c);
    }

    // Shaft + four-facet head, pointing from base along dir for len metres.
    void arrow(const Vec3& base, const Vec3& dir, double len, double thick, const Color& c) {
        const Vec3 d = normalized(dir);
        const double headLen = len * 0.22;
        const Vec3 tip = base + d * len;
        const Vec3 neck = base + d * (len - headLen);
        bar(base, neck, thick, c);

        Vec3 ref = (std::fabs(d.z) < 0.9) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        const Vec3 u = normalized(cross(d, ref)) * (thick * 2.2);
        const Vec3 v = normalized(cross(d, u)) * (thick * 2.2);
        const Vec3 ring[4] = {neck + u, neck + v, neck - u, neck - v};
        for (int k = 0; k < 4; ++k) {
            tri(ring[k], ring[(k + 1) % 4], tip, c);
            tri(ring[(k + 1) % 4], ring[k], neck, c);
        }
    }

    // Closed polyline on the circle of radius R spanned by orthonormal u, v.
    void ring(const Vec3& centre, const Vec3& u, const Vec3& v, double R, int segments,
              const Color& c) {
        Vec3 prev = centre + u * R;
        for (int k = 1; k <= segments; ++k) {
            const double t = 2.0 * M_PI * k / segments;
            const Vec3 p = centre + u * (R * std::cos(t)) + v * (R * std::sin(t));
            line(prev, p, c);
            prev = p;
        }
    }

    // Translucent UV-sphere shell, origin-centred.
    void sphereFaces(double R, int nLon, int nLat, const Color& c) {
        for (int a = 0; a < nLat; ++a) {
            const double th0 = M_PI * a / nLat;
            const double th1 = M_PI * (a + 1) / nLat;
            for (int b = 0; b < nLon; ++b) {
                const double ph0 = 2.0 * M_PI * b / nLon;
                const double ph1 = 2.0 * M_PI * (b + 1) / nLon;
                const Vec3 p00 = sph(R, th0, ph0), p01 = sph(R, th0, ph1);
                const Vec3 p11 = sph(R, th1, ph1), p10 = sph(R, th1, ph0);
                // Degenerate at the poles, which is harmless here.
                quad(p00, p01, p11, p10, c, true);
            }
        }
    }

    // Three great circles -- enough to read the sphere's size and orientation
    // without the clutter of a full wireframe.
    void sphereWire(double R, int segments, const Color& c) {
        const Vec3 ex{1, 0, 0}, ey{0, 1, 0}, ez{0, 0, 1};
        ring({0, 0, 0}, ex, ey, R, segments, c);  // equatorial, xy
        ring({0, 0, 0}, ex, ez, R, segments, c);  // meridian, xz
        ring({0, 0, 0}, ey, ez, R, segments, c);  // meridian, yz
    }

    const std::vector<Vertex>& lines() const { return lines_; }
    const std::vector<Vertex>& opaqueTris() const { return opaque_; }
    const std::vector<Vertex>& blendedTris() const { return blended_; }

private:
    static Vec3 sph(double R, double theta, double phi) {
        return {R * std::sin(theta) * std::cos(phi),
                R * std::sin(theta) * std::sin(phi),
                R * std::cos(theta)};
    }

    static void push(std::vector<Vertex>& v, const Vec3& p, const Color& c) {
        v.push_back({(float)p.x, (float)p.y, (float)p.z, c.r, c.g, c.b, c.a});
    }

    std::vector<Vertex> lines_, opaque_, blended_;
};

// ---------------------------------------------------------------------------

class Renderer {
public:
    bool init(std::string* err);
    void shutdown();
    void draw(const Batch& batch, const Mat4& viewProj);

private:
    void upload(const std::vector<Vertex>& v);

    GLuint prog_ = 0, vao_ = 0, vbo_ = 0;
    GLint uMVP_ = -1;
};

}  // namespace hc
