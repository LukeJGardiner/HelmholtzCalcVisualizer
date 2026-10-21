// gl_core.hpp -- the ~35 OpenGL 3.3 entry points this project actually uses,
// loaded through glfwGetProcAddress.
//
// Why not glad/GLEW: both are extra FetchContent surface area for a program
// that draws lines and triangles.  Dear ImGui's GL3 backend ships its own
// loader in its own translation unit, so nothing here can collide with it --
// but only as long as no GL system header is included in OUR units.  Hence
// GLFW_INCLUDE_NONE everywhere, and every type/constant declared below.
#pragma once

#include <cstddef>

// ------------------------------------------------------------- types -------
using GLenum     = unsigned int;
using GLbitfield = unsigned int;
using GLuint     = unsigned int;
using GLint      = int;
using GLsizei    = int;
using GLboolean  = unsigned char;
using GLfloat    = float;
using GLchar     = char;
using GLvoid     = void;
using GLsizeiptr = std::ptrdiff_t;
using GLintptr   = std::ptrdiff_t;

// --------------------------------------------------------- constants -------
#define GL_FALSE                 0
#define GL_TRUE                  1
#define GL_POINTS                0x0000
#define GL_LINES                 0x0001
#define GL_TRIANGLES             0x0004
#define GL_LESS                  0x0201
#define GL_DEPTH_BUFFER_BIT      0x00000100
#define GL_COLOR_BUFFER_BIT      0x00004000
#define GL_SRC_ALPHA             0x0302
#define GL_ONE_MINUS_SRC_ALPHA   0x0303
#define GL_CULL_FACE             0x0B44
#define GL_DEPTH_TEST            0x0B71
#define GL_BLEND                 0x0BE2
#define GL_LINE_SMOOTH           0x0B20
#define GL_FLOAT                 0x1406
#define GL_RENDERER              0x1F01
#define GL_VERSION               0x1F02
#define GL_MULTISAMPLE           0x809D
#define GL_ARRAY_BUFFER          0x8892
#define GL_DYNAMIC_DRAW          0x88E8
#define GL_FRAGMENT_SHADER       0x8B30
#define GL_VERTEX_SHADER         0x8B31
#define GL_COMPILE_STATUS        0x8B81
#define GL_LINK_STATUS           0x8B82

// -------------------------------------------------- function pointers ------
#define HC_GL_FUNCTIONS(X)                                                              \
    X(void,   Clear,                   (GLbitfield))                                    \
    X(void,   ClearColor,              (GLfloat, GLfloat, GLfloat, GLfloat))            \
    X(void,   Enable,                  (GLenum))                                        \
    X(void,   Disable,                 (GLenum))                                        \
    X(void,   DepthFunc,               (GLenum))                                        \
    X(void,   DepthMask,               (GLboolean))                                     \
    X(void,   BlendFunc,               (GLenum, GLenum))                                \
    X(void,   Viewport,                (GLint, GLint, GLsizei, GLsizei))                \
    X(void,   DrawArrays,              (GLenum, GLint, GLsizei))                        \
    X(const unsigned char*, GetString, (GLenum))                                        \
    X(void,   GenVertexArrays,         (GLsizei, GLuint*))                              \
    X(void,   BindVertexArray,         (GLuint))                                        \
    X(void,   DeleteVertexArrays,      (GLsizei, const GLuint*))                        \
    X(void,   GenBuffers,              (GLsizei, GLuint*))                              \
    X(void,   BindBuffer,              (GLenum, GLuint))                                \
    X(void,   BufferData,              (GLenum, GLsizeiptr, const void*, GLenum))       \
    X(void,   DeleteBuffers,           (GLsizei, const GLuint*))                        \
    X(void,   EnableVertexAttribArray, (GLuint))                                        \
    X(void,   VertexAttribPointer,     (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)) \
    X(GLuint, CreateShader,            (GLenum))                                        \
    X(void,   ShaderSource,            (GLuint, GLsizei, const GLchar* const*, const GLint*)) \
    X(void,   CompileShader,           (GLuint))                                        \
    X(void,   GetShaderiv,             (GLuint, GLenum, GLint*))                        \
    X(void,   GetShaderInfoLog,        (GLuint, GLsizei, GLsizei*, GLchar*))            \
    X(void,   DeleteShader,            (GLuint))                                        \
    X(GLuint, CreateProgram,           ())                                              \
    X(void,   AttachShader,            (GLuint, GLuint))                                \
    X(void,   LinkProgram,             (GLuint))                                        \
    X(void,   GetProgramiv,            (GLuint, GLenum, GLint*))                        \
    X(void,   GetProgramInfoLog,       (GLuint, GLsizei, GLsizei*, GLchar*))            \
    X(void,   DeleteProgram,           (GLuint))                                        \
    X(void,   UseProgram,              (GLuint))                                        \
    X(GLint,  GetUniformLocation,      (GLuint, const GLchar*))                         \
    X(void,   UniformMatrix4fv,        (GLint, GLsizei, GLboolean, const GLfloat*))

#define HC_DECLARE_GL(ret, name, args) \
    using PFN_gl##name = ret (*) args; \
    extern PFN_gl##name gl##name;
HC_GL_FUNCTIONS(HC_DECLARE_GL)
#undef HC_DECLARE_GL

// Loads every pointer above.  Returns the number that failed to resolve.
int hcLoadGL(void* (*getProcAddress)(const char*));
