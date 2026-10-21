#include "renderer.hpp"

#include <cstddef>

namespace hc {

static const char* kVert = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aCol;
uniform mat4 uMVP;
out vec4 vCol;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vCol = aCol;
}
)";

static const char* kFrag = R"(#version 330 core
in vec4 vCol;
out vec4 fragColor;
void main() { fragColor = vCol; }
)";

static GLuint compile(GLenum stage, const char* src, std::string* err) {
    const GLuint sh = glCreateShader(stage);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048] = {0};
        glGetShaderInfoLog(sh, sizeof(log) - 1, nullptr, log);
        if (err) *err = std::string("shader compile failed: ") + log;
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

bool Renderer::init(std::string* err) {
    const GLuint vs = compile(GL_VERTEX_SHADER, kVert, err);
    if (!vs) return false;
    const GLuint fs = compile(GL_FRAGMENT_SHADER, kFrag, err);
    if (!fs) {
        glDeleteShader(vs);
        return false;
    }

    prog_ = glCreateProgram();
    glAttachShader(prog_, vs);
    glAttachShader(prog_, fs);
    glLinkProgram(prog_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048] = {0};
        glGetProgramInfoLog(prog_, sizeof(log) - 1, nullptr, log);
        if (err) *err = std::string("program link failed: ") + log;
        return false;
    }

    uMVP_ = glGetUniformLocation(prog_, "uMVP");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void*)offsetof(Vertex, r));
    glBindVertexArray(0);
    return true;
}

void Renderer::shutdown() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (prog_) glDeleteProgram(prog_);
    vbo_ = vao_ = prog_ = 0;
}

void Renderer::upload(const std::vector<Vertex>& v) {
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(v.size() * sizeof(Vertex)),
                 v.data(), GL_DYNAMIC_DRAW);
}

void Renderer::draw(const Batch& batch, const Mat4& viewProj) {
    glUseProgram(prog_);
    glUniformMatrix4fv(uMVP_, 1, GL_FALSE, viewProj.m);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Pass 1 -- solid geometry (coil conductors, field arrow), depth written.
    glDepthMask(GL_TRUE);
    if (!batch.opaqueTris().empty()) {
        upload(batch.opaqueTris());
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)batch.opaqueTris().size());
    }

    // Pass 2 -- wireframes, also depth written.
    if (!batch.lines().empty()) {
        upload(batch.lines());
        glDrawArrays(GL_LINES, 0, (GLsizei)batch.lines().size());
    }

    // Pass 3 -- translucent faces: depth-tested but not depth-writing, so the
    // stable-region shell reads as glass rather than hiding what it contains.
    glDepthMask(GL_FALSE);
    if (!batch.blendedTris().empty()) {
        upload(batch.blendedTris());
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)batch.blendedTris().size());
    }
    glDepthMask(GL_TRUE);

    glBindVertexArray(0);
    glUseProgram(0);
}

}  // namespace hc
