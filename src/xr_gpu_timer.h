#pragma once
#ifdef SCU_QUEST
#include <SDL.h>
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <GLES2/gl2ext.h>
#include <array>
#include <cstring>

namespace scu {
// Optional GPU feedback. Never wait for a query result; a full ring simply
// skips measurement. Disjoint samples and stale results never drive adaptation.
class XrGpuTimer {
    PFNGLGENQUERIESEXTPROC generate = nullptr;
    PFNGLDELETEQUERIESEXTPROC remove = nullptr;
    PFNGLBEGINQUERYEXTPROC begin = nullptr;
    PFNGLENDQUERYEXTPROC end = nullptr;
    PFNGLGETQUERYOBJECTUIVEXTPROC status = nullptr;
    PFNGLGETQUERYOBJECTUI64VEXTPROC result = nullptr;
    PFNGLGETQUERYIVEXTPROC binding = nullptr;
    struct Pair {
        std::array<GLuint, 2> ids{};
        unsigned pending = 0;
        bool valid = true;
        uint64_t sequence = 0;
    };
    std::array<Pair, 4> pairs{};
    int current = -1, active = -1;
    double milliseconds = -1;
    uint64_t measuredAt = 0;
    uint64_t nextSequence = 0, lastSequence = 0;

  public:
    void initialise() {
        const auto *extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
        if (!extensions || !std::strstr(extensions, "GL_EXT_disjoint_timer_query"))
            return;
        generate = reinterpret_cast<PFNGLGENQUERIESEXTPROC>(eglGetProcAddress("glGenQueriesEXT"));
        remove =
            reinterpret_cast<PFNGLDELETEQUERIESEXTPROC>(eglGetProcAddress("glDeleteQueriesEXT"));
        begin = reinterpret_cast<PFNGLBEGINQUERYEXTPROC>(eglGetProcAddress("glBeginQueryEXT"));
        end = reinterpret_cast<PFNGLENDQUERYEXTPROC>(eglGetProcAddress("glEndQueryEXT"));
        status = reinterpret_cast<PFNGLGETQUERYOBJECTUIVEXTPROC>(
            eglGetProcAddress("glGetQueryObjectuivEXT"));
        result = reinterpret_cast<PFNGLGETQUERYOBJECTUI64VEXTPROC>(
            eglGetProcAddress("glGetQueryObjectui64vEXT"));
        binding = reinterpret_cast<PFNGLGETQUERYIVEXTPROC>(eglGetProcAddress("glGetQueryivEXT"));
        if (!(generate && remove && begin && end && status && result && binding)) {
            generate = nullptr;
            return;
        }
        GLint counterBits = 0;
        binding(GL_TIME_ELAPSED_EXT, GL_QUERY_COUNTER_BITS_EXT, &counterBits);
        if (counterBits <= 0 || counterBits > 64) {
            generate = nullptr;
            return;
        }
        for (auto &p : pairs)
            generate(2, p.ids.data());
    }
    void frame() {
        current = -1;
        if (!generate)
            return;
        GLboolean disjoint = GL_FALSE;
        glGetBooleanv(GL_GPU_DISJOINT_EXT, &disjoint);
        if (disjoint) {
            milliseconds = -1;
            measuredAt = 0;
            for (auto &p : pairs)
                if (p.pending)
                    p.valid = false;
        }
        for (auto &p : pairs)
            if (p.pending) {
                bool ready = true;
                for (int eye = 0; eye < 2; ++eye)
                    if (p.pending & (1u << eye)) {
                        GLuint available = 0;
                        status(p.ids[eye], GL_QUERY_RESULT_AVAILABLE_EXT, &available);
                        ready &= available != 0;
                    }
                if (!ready)
                    continue;
                if (!disjoint && p.valid && p.pending == 3 && p.sequence > lastSequence) {
                    GLuint64 total = 0;
                    for (auto id : p.ids) {
                        GLuint64 ns = 0;
                        result(id, GL_QUERY_RESULT_EXT, &ns);
                        total += ns;
                    }
                    double value = total * 1e-6;
                    if (value >= 0 && value < 250) {
                        milliseconds = value;
                        measuredAt = SDL_GetTicks64();
                        lastSequence = p.sequence;
                    }
                }
                p.pending = 0;
            }
        for (int i = 0; i < int(pairs.size()); ++i)
            if (!pairs[i].pending) {
                current = i;
                pairs[i].valid = !disjoint;
                pairs[i].sequence = ++nextSequence;
                break;
            }
    }
    void startEye(int eye) {
        if (current < 0 || active >= 0 || eye < 0 || eye > 1)
            return;
        GLint inUse = 0;
        binding(GL_TIME_ELAPSED_EXT, GL_CURRENT_QUERY_EXT, &inUse);
        if (inUse)
            return;
        begin(GL_TIME_ELAPSED_EXT, pairs[current].ids[eye]);
        active = eye;
    }
    void finishEye() {
        if (active < 0)
            return;
        end(GL_TIME_ELAPSED_EXT);
        pairs[current].pending |= 1u << active;
        active = -1;
    }
    double latest() const {
        return measuredAt && SDL_GetTicks64() - measuredAt < 500 ? milliseconds : -1;
    }
    ~XrGpuTimer() {
        finishEye();
        if (generate && remove)
            for (auto &p : pairs)
                remove(2, p.ids.data());
    }
};
} // namespace scu
#endif
