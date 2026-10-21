#include "gl_core.hpp"

#include <cstdio>

#define HC_DEFINE_GL(ret, name, args) PFN_gl##name gl##name = nullptr;
HC_GL_FUNCTIONS(HC_DEFINE_GL)
#undef HC_DEFINE_GL

int hcLoadGL(void* (*getProcAddress)(const char*)) {
    int missing = 0;

#define HC_LOAD_GL(ret, name, args)                                        \
    gl##name = reinterpret_cast<PFN_gl##name>(getProcAddress("gl" #name)); \
    if (!gl##name) {                                                       \
        std::fprintf(stderr, "[gl] could not resolve gl%s\n", #name);      \
        ++missing;                                                         \
    }
    HC_GL_FUNCTIONS(HC_LOAD_GL)
#undef HC_LOAD_GL

    return missing;
}
