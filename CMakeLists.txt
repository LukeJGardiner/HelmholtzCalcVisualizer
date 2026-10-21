cmake_minimum_required(VERSION 3.16)
project(helmholtz_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

include(FetchContent)

# ---------------------------------------------------------------- GLFW -----
# Wayland is switched off on purpose: under WSLg the X11 path is the reliable
# one, and leaving Wayland on makes the build demand wayland-protocols etc.
set(GLFW_BUILD_DOCS      OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL         OFF CACHE BOOL "" FORCE)
if(UNIX AND NOT APPLE)
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_X11     ON  CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(glfw)

# ----------------------------------------------------------- Dear ImGui ----
# ImGui ships no CMakeLists, so MakeAvailable just populates and we compile
# the handful of sources ourselves.
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.5
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)

# ------------------------------------------------------------- target ------
add_executable(helmholtz_sim
  src/main.cpp
  src/renderer.cpp
  src/gl_core.cpp)
target_include_directories(helmholtz_sim PRIVATE src)
target_link_libraries(helmholtz_sim PRIVATE imgui glfw)

find_package(Threads REQUIRED)
target_link_libraries(helmholtz_sim PRIVATE Threads::Threads)
if(UNIX AND NOT APPLE)
  target_link_libraries(helmholtz_sim PRIVATE ${CMAKE_DL_LIBS})
endif()

if(MSVC)
  target_compile_options(helmholtz_sim PRIVATE /W4)
else()
  target_compile_options(helmholtz_sim PRIVATE -Wall -Wextra)
endif()
