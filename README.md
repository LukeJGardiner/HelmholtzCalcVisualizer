# Helmholtz coil testbed

Square Helmholtz pair at the optimum separation. Reports the magnetic flux
density at the origin and the radius of the stable spherical region around it,
with a 3D view of the coils and that sphere.

## Relations

Two closed forms drive everything. Both are evaluated directly; there is no
numerical field solve anywhere in the program.

**Field at the origin** — Batista et al., *Three-Axial Helmholtz Coil Design and
Validation for Aerospace Applications*, IEEE TAES 54(1), 2018, eq. (13):

```
|B(0)| = 1.628733676e-6 * N * i / L     [T],   direction +z for i > 0
```

valid at the optimum separation of eq. (12), `d = 2·zA = σ·L` with
`σ = 0.544505643` — the root of `6σ⁶ + 18σ⁴ + 11σ² − 5` that nulls the second
derivative of the centre field with respect to separation.

**Stable region.** The centre of the pair is stable within a sphere whose
diameter is half the coil separation:

```
diameter = d / 2 = σ·L / 2
radius   = d / 4 = σ·L / 4 = 0.136126411 · L
```

Purely geometric: it scales with `L` and does not depend on `N`, `i`, or any
tolerance figure.

Worked values at N=63, i=1.76 A:

| L (m) | d (m) | radius (m) | radius (cm) | \|B(0)\| (G) |
|---|---|---|---|---|
| 0.20 | 0.10890 | 0.02723 | 2.72 | 9.0297 |
| 0.88 | 0.47916 | 0.11979 | 11.98 | 2.0522 |
| 2.00 | 1.08901 | 0.27225 | 27.23 | 0.9030 |
| 4.00 | 2.17802 | 0.54451 | 54.45 | 0.4515 |

## Build (WSL2 / Ubuntu)

One-time prerequisites:

```bash
sudo apt update && sudo apt install -y build-essential cmake git pkg-config \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libgl1-mesa-dev mesa-utils
```

Confirm you have a display and GL: `echo $DISPLAY` should print something like
`:0`, and `glxinfo -B` should succeed.

Then:

```bash
cmake -S . -B build
cmake --build build -j
./build/helmholtz_sim
```

GLFW 3.4 and Dear ImGui v1.91.5 are fetched on first configure (requires
network). Wayland is disabled in the GLFW build on purpose — X11 is the
dependable path under WSLg.

No glad or GLEW: `gl_core.hpp` loads the ~35 GL 3.3 entry points this program
uses through `glfwGetProcAddress`.

**If CMake reports `Cannot find source file: src/main.cpp`**, the sources are
sitting flat in the project root rather than in `src/`. Fix with
`mkdir -p src && mv *.cpp *.hpp src/`.

## Using it

Three live parameters:

| | range |
|---|---|
| `L` coil side | 0.2 – 4 m |
| `N` turns per coil | 1 – 50 |
| `i` current | 0.1 – 20 A, log slider |

Results update every frame. The panel reports `|B(0)|` in µT, mG and G, the
coil separation and plane positions, and the stable sphere as a radius, a
diameter, and explicit bounds.

*Paper prototype (inner pair)* loads L=0.88 m, N=63, i=1.76 A for a quick check
against Table II of the paper (2.052 G).

**Navigation**: left-drag orbits, right-drag pans, wheel zooms, *Reset view*
re-frames. The *View* section toggles the coils, the sphere, its translucent
shell, the B arrow, the axes and the ground grid independently.

**CSV log**: *Append current state* writes a row to `helmholtz_log.csv` in the
working directory (override with `--log path.csv`); *auto on release* logs each
time you let go of a slider. Columns: timestamp, L, N, i, |B(0)| in µT and G,
direction, separation, coil plane z, stable radius and diameter.

## Layout

```
src/physics.hpp         the two closed forms
src/linalg.hpp          Vec3 / Mat4
src/camera.hpp          orbit camera
src/gl_core.{hpp,cpp}   minimal GL 3.3 loader
src/renderer.{hpp,cpp}  line/triangle batcher, three-pass draw
src/csvlog.hpp          append-only logger
src/main.cpp            ImGui panel, scene, main loop
```

## Notes and limits

* Coils are modelled as ideal filaments carrying `N·i`; no conductor bundle
  cross-section, winding pitch, or lead-in effects. At N=50 on a small coil the
  bundle is not negligible against `L` and a real coil will differ slightly.
* Single pair on the z-axis, not the full triaxial cage. The paper's middle and
  outer pairs are the same relations with `L` increased by twice the structural
  profile thickness.
* The stable-sphere radius is a geometric rule of thumb, not a tolerance-driven
  result. It says nothing by itself about how many parts per thousand the field
  varies inside that sphere.
