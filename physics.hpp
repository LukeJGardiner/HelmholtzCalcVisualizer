// physics.hpp -- square Helmholtz pair: field at the origin and the extent of
// the stable region around it.
//
// Source of the relations
// -----------------------
// D. S. Batista, F. Granziera Jr., M. C. Tosin, L. F. de Melo,
// "Three-Axial Helmholtz Coil Design and Validation for Aerospace
//  Applications", IEEE Trans. Aerospace and Electronic Systems,
//  vol. 54, no. 1, pp. 392-403, Feb. 2018.
//
//   eq. (12)  optimum separation   d = 2*zA = sigma * L,  sigma = 0.544505643
//   eq. (13)  |B(0)| = 1.628733676e-6 * N * i / L      [T]
//
// Stable region
// -------------
// The centre of a Helmholtz pair is taken to be stable within a sphere whose
// DIAMETER is half the coil separation d.  With d = sigma*L:
//
//   diameter = d / 2     = sigma * L / 2
//   radius   = d / 4     = sigma * L / 4   = 0.136126411 * L
//
// This is a purely geometric criterion: it scales with L and has no dependence
// on N, i, or any uniformity tolerance.
#pragma once

#include <cmath>

#include "linalg.hpp"

namespace hc {

// --------------------------------------------------------- constants -------

// Separation parameter -- the root of 6s^6 + 18s^4 + 11s^2 - 5 that nulls the
// second derivative of the centre field with respect to separation, paper
// eq. (11), giving the optimum spacing of eq. (12).
inline constexpr double SIGMA = 0.544505643;

// Field constant of paper eq. (13), in [T*m/(A*turn)].
inline constexpr double K_FIELD = 1.628733676e-6;

// radius = K_RADIUS * L, from diameter = d/2 and d = sigma*L.
inline constexpr double K_RADIUS = SIGMA / 4.0;

// Live parameter limits.
inline constexpr double L_MIN = 0.20, L_MAX = 4.00;  // m
inline constexpr int    N_MIN = 1,    N_MAX = 50;    // turns
inline constexpr double I_MIN = 0.10, I_MAX = 20.0;  // A

// ------------------------------------------------------------ results ------

// Centre-to-centre separation of the two coils [m] -- eq. (12).
inline double coilSeparation(double L) { return SIGMA * L; }

// Axial position of each coil plane, z = +/- this [m].
inline double coilPlaneZ(double L) { return 0.5 * SIGMA * L; }

// |B| at the origin [T] -- eq. (13).  Direction is +z for positive current.
inline double fieldAtOriginT(double L, double N, double i) {
    return K_FIELD * N * i / L;
}

// Radius [m] of the stable spherical region centred on the origin.
inline double stableRadius(double L) { return K_RADIUS * L; }

// Everything the UI, the renderer and the CSV logger need for one parameter set.
struct Solution {
    // Inputs, echoed back.
    double L = 1.0;  // m
    int    N = 1;    // turns
    double i = 1.0;  // A

    // Field at the origin.
    double B0_T = 0.0;   // tesla
    double B0_uT = 0.0;  // microtesla
    double B0_G = 0.0;   // gauss (1 T = 1e4 G)
    Vec3   B0_dir{0.0, 0.0, 1.0};

    // Geometry.
    double separation = 0.0;  // m, coil-to-coil
    double coilZ = 0.0;       // m, |z| of each coil plane

    // Stable region.
    double radius = 0.0;    // m
    double diameter = 0.0;  // m
};

inline Solution solve(double L, int N, double i) {
    Solution s;
    s.L = L;
    s.N = N;
    s.i = i;

    s.B0_T  = fieldAtOriginT(L, (double)N, i);
    s.B0_uT = s.B0_T * 1.0e6;
    s.B0_G  = s.B0_T * 1.0e4;
    s.B0_dir = Vec3{0.0, 0.0, (i >= 0.0) ? 1.0 : -1.0};

    s.separation = coilSeparation(L);
    s.coilZ      = coilPlaneZ(L);

    s.radius   = stableRadius(L);
    s.diameter = 2.0 * s.radius;
    return s;
}

}  // namespace hc
