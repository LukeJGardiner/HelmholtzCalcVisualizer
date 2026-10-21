// main.cpp -- Helmholtz coil testbed calculator.
//
// Three live parameters (L, N, i) drive two closed-form results: the flux
// density at the origin, and the radius of the stable spherical region around
// it.  Both are a handful of flops, so everything recomputes every frame and
// the 3D view follows the sliders immediately.
//
// Physics lives entirely in physics.hpp; nothing below computes a field.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "camera.hpp"
#include "csvlog.hpp"
#include "gl_core.hpp"
#include "linalg.hpp"
#include "physics.hpp"
#include "renderer.hpp"

using namespace hc;

namespace {

// ------------------------------------------------------------ palette -----
const Color kColCoil        {0.92f, 0.62f, 0.18f, 1.00f};
const Color kColSphereWire  {0.25f, 0.90f, 0.85f, 1.00f};
const Color kColSphereShell {0.20f, 0.80f, 0.78f, 0.11f};
const Color kColAxisX       {0.85f, 0.30f, 0.33f, 1.00f};
const Color kColAxisY       {0.40f, 0.78f, 0.35f, 1.00f};
const Color kColAxisZ       {0.38f, 0.60f, 0.95f, 1.00f};
const Color kColGrid        {0.26f, 0.28f, 0.33f, 1.00f};
const Color kColField       {0.95f, 0.95f, 0.98f, 1.00f};

struct AppState {
    // Live parameters.
    float L = 0.88f;  // m
    int   N = 63;     // turns
    float i = 1.76f;  // A

    // View options.
    bool showGrid = true;
    bool showAxes = true;
    bool showCoils = true;
    bool showSphere = true;
    bool showShell = true;
    bool showFieldArrow = true;
    bool autoLog = false;

    OrbitCamera cam;
    bool dragOrbit = false, dragPan = false;
    double lastX = 0.0, lastY = 0.0;
};

AppState g;
CsvLog g_log("helmholtz_log.csv");

// --------------------------------------------------------- scene build ----

void buildGrid(Batch& b, double extent) {
    // A faint ground plane below the coils, purely for depth perception
    // while orbiting.
    const double z = -extent;
    const int n = 10;
    const double step = extent / n;
    for (int k = -n; k <= n; ++k) {
        const double t = k * step;
        b.line({t, -extent, z}, {t, extent, z}, kColGrid);
        b.line({-extent, t, z}, {extent, t, z}, kColGrid);
    }
}

void buildAxes(Batch& b, double len) {
    b.line({0, 0, 0}, {len, 0, 0}, kColAxisX);
    b.line({0, 0, 0}, {0, len, 0}, kColAxisY);
    b.line({0, 0, 0}, {0, 0, len}, kColAxisZ);
}

void buildCoils(Batch& b, double L, double thickness) {
    const double h = 0.5 * L;
    const double zc = coilPlaneZ(L);
    for (double z : {-zc, zc}) {
        const Vec3 c0{-h, -h, z}, c1{h, -h, z}, c2{h, h, z}, c3{-h, h, z};
        b.bar(c0, c1, thickness, kColCoil);
        b.bar(c1, c2, thickness, kColCoil);
        b.bar(c2, c3, thickness, kColCoil);
        b.bar(c3, c0, thickness, kColCoil);
    }
}

double sceneExtent(const Solution& s) { return 0.5 * s.L; }

void buildScene(Batch& b, const Solution& s) {
    b.clear();
    const double ext = sceneExtent(s);

    if (g.showGrid) buildGrid(b, ext * 1.6);
    if (g.showAxes) buildAxes(b, ext * 1.15);
    if (g.showCoils) buildCoils(b, s.L, std::max(s.L / 90.0, 1e-4));

    if (g.showSphere) {
        if (g.showShell) b.sphereFaces(s.radius, 40, 24, kColSphereShell);
        b.sphereWire(s.radius, 72, kColSphereWire);
    }

    if (g.showFieldArrow) {
        const double len = std::max(s.radius * 1.6, ext * 0.25);
        b.arrow({0, 0, 0}, s.B0_dir, len, std::max(s.L / 110.0, 1e-4), kColField);
    }
}

// ------------------------------------------------------------- input ------

void handleMouse(GLFWwindow* win) {
    ImGuiIO& io = ImGui::GetIO();
    double x, y;
    glfwGetCursorPos(win, &x, &y);
    const double dx = x - g.lastX, dy = y - g.lastY;
    g.lastX = x;
    g.lastY = y;

    const bool left  = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool right = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool mid   = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

    // Only begin a drag outside ImGui, but let an in-progress drag continue
    // even if the cursor wanders over a panel.
    if (!io.WantCaptureMouse) {
        if (left && !g.dragPan) g.dragOrbit = true;
        if ((right || mid) && !g.dragOrbit) g.dragPan = true;
    }
    if (!left) g.dragOrbit = false;
    if (!right && !mid) g.dragPan = false;

    int fbw, fbh;
    glfwGetFramebufferSize(win, &fbw, &fbh);
    if (g.dragOrbit) g.cam.orbit(dx, dy);
    if (g.dragPan) g.cam.pan(dx, dy, fbh);
}

void scrollCallback(GLFWwindow*, double, double yoff) {
    if (!ImGui::GetIO().WantCaptureMouse) g.cam.dolly(yoff);
}

// ---------------------------------------------------------------- UI ------

void controlWindow(const Solution& s) {
    ImGui::SetNextWindowPos(ImVec2(16, 16), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 560), ImGuiCond_FirstUseEver);
    ImGui::Begin("Helmholtz coil");

    // ---- live parameters ----
    ImGui::SeparatorText("Parameters");

    ImGui::SliderFloat("L  coil side (m)", &g.L, (float)L_MIN, (float)L_MAX, "%.3f");
    ImGui::SliderInt("N  turns per coil", &g.N, N_MIN, N_MAX);
    ImGui::SliderFloat("i  current (A)", &g.i, (float)I_MIN, (float)I_MAX, "%.3f",
                       ImGuiSliderFlags_Logarithmic);

    if (ImGui::SmallButton("Paper prototype (inner pair)")) {
        g.L = 0.88f;
        g.N = 63;
        g.i = 1.76f;
    }

    // ---- field at the origin ----
    ImGui::SeparatorText("Field at the origin");
    ImGui::Text("|B|  = %10.4f  uT", s.B0_uT);
    ImGui::Text("     = %10.4f  mG", s.B0_G * 1000.0);
    ImGui::Text("     = %10.6f  G", s.B0_G);
    ImGui::Text("direction  %s  (along the coil axis)", s.B0_dir.z >= 0 ? "+z" : "-z");
    ImGui::TextDisabled("eq.(13):  |B| = 1.628733676e-6 * N * i / L   [T]");

    // ---- geometry ----
    ImGui::SeparatorText("Coil geometry");
    ImGui::Text("separation   d = %.5f m", s.separation);
    ImGui::Text("coil planes  z = +/- %.5f m", s.coilZ);
    ImGui::TextDisabled("d = sigma * L,  sigma = %.9f", SIGMA);

    // ---- stable region ----
    ImGui::SeparatorText("Stable region (sphere)");
    ImGui::TextDisabled("diameter = d / 2   ->   radius = d / 4 = %.9f * L", K_RADIUS);
    ImGui::Spacing();
    ImGui::Text("radius    = %.5f m   (%.2f cm)", s.radius, s.radius * 100.0);
    ImGui::Text("diameter  = %.5f m   (%.2f cm)", s.diameter, s.diameter * 100.0);
    ImGui::Spacing();
    ImGui::Text("bounds    x, y, z = +/- %.5f m", s.radius);

    // ---- logging ----
    ImGui::SeparatorText("CSV log");
    if (ImGui::Button("Append current state")) g_log.append(s);
    ImGui::SameLine();
    ImGui::Checkbox("auto on release", &g.autoLog);
    ImGui::TextDisabled("%s  (%ld row%s)", g_log.path().c_str(), g_log.rows(),
                        g_log.rows() == 1 ? "" : "s");
    if (!g_log.lastError().empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", g_log.lastError().c_str());
    }

    // ---- view ----
    ImGui::SeparatorText("View");
    ImGui::Checkbox("coils", &g.showCoils);
    ImGui::SameLine();
    ImGui::Checkbox("stable sphere", &g.showSphere);
    ImGui::Checkbox("sphere shell", &g.showShell);
    ImGui::SameLine();
    ImGui::Checkbox("B arrow", &g.showFieldArrow);
    ImGui::Checkbox("axes", &g.showAxes);
    ImGui::SameLine();
    ImGui::Checkbox("grid", &g.showGrid);
    if (ImGui::Button("Reset view")) g.cam.frame(sceneExtent(s));
    ImGui::TextDisabled("left drag orbit | right drag pan | wheel zoom");

    ImGui::End();
}

}  // namespace

int main(int argc, char** argv) {
    for (int k = 1; k < argc; ++k) {
        const std::string a = argv[k];
        if (a == "--log" && k + 1 < argc) g_log = CsvLog(argv[++k]);
    }

    glfwSetErrorCallback([](int code, const char* desc) {
        std::fprintf(stderr, "[glfw] error %d: %s\n", code, desc);
    });
    if (!glfwInit()) {
        std::fprintf(stderr,
                     "Failed to initialise GLFW. Under WSL this usually means no display: "
                     "check that $DISPLAY is set and WSLg is available.\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* win = glfwCreateWindow(1440, 900, "Helmholtz coil testbed", nullptr, nullptr);
    if (!win) {
        std::fprintf(stderr, "Failed to create an OpenGL 3.3 core window.\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    glfwSetScrollCallback(win, scrollCallback);

    if (hcLoadGL(reinterpret_cast<void* (*)(const char*)>(glfwGetProcAddress)) != 0) {
        std::fprintf(stderr, "Failed to load the required OpenGL 3.3 entry points.\n");
        glfwTerminate();
        return 1;
    }
    std::printf("[gl] %s | %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;  // no imgui.ini clutter
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 4.0f;
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Renderer renderer;
    std::string err;
    if (!renderer.init(&err)) {
        std::fprintf(stderr, "Renderer init failed: %s\n", err.c_str());
        return 1;
    }

    g.cam.frame(0.5 * g.L);
    glfwGetCursorPos(win, &g.lastX, &g.lastY);

    Batch batch;
    bool prevAnyActive = false;

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        handleMouse(win);

        // The closed forms cost a few dozen flops, so there is nothing to gain
        // by deferring: recompute every frame.
        const Solution s = solve(g.L, g.N, g.i);
        controlWindow(s);
        buildScene(batch, s);

        // "Auto on release": log the frame after the last widget is let go.
        const bool anyActive = ImGui::IsAnyItemActive();
        if (g.autoLog && prevAnyActive && !anyActive) g_log.append(s);
        prevAnyActive = anyActive;

        int fbw, fbh;
        glfwGetFramebufferSize(win, &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
        glClearColor(0.07f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_MULTISAMPLE);

        renderer.draw(batch, g.cam.viewProj(fbw, fbh));

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    renderer.shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
