#include "voxel_gpu.h"
#include "voxel_shaders.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>

namespace scu {
namespace {
#ifdef _WIN32
#define SCU_GL_CALL __stdcall
#else
#define SCU_GL_CALL
#endif
using U = unsigned;
using I = int;
using B = unsigned char;
constexpr U tex2D = 0x0DE1, texture0 = 0x84C0, arrayBuffer = 0x8892, framebuffer = 0x8D40,
            renderbuffer = 0x8D41, blend = 0x0BE2, depthTest = 0x0B71, scissorTest = 0x0C11,
            cullFace = 0x0B44, rgba = 0x1908, ubyte = 0x1401, floatType = 0x1406,
            triangles = 0x0004;
struct Gl {
#define FN(R, N, P) R(SCU_GL_CALL *N) P = nullptr
    FN(void, GetIntegerv, (U, I *));
    FN(void, GetFloatv, (U, float *));
    FN(void, GetBooleanv, (U, B *));
    FN(const unsigned char *, GetString, (U));
    FN(B, IsEnabled, (U));
    FN(void, Enable, (U));
    FN(void, Disable, (U));
    FN(void, ActiveTexture, (U));
    FN(void, BindTexture, (U, U));
    FN(void, GenTextures, (I, U *));
    FN(void, DeleteTextures, (I, const U *));
    FN(void, TexParameteri, (U, U, I));
    FN(void, TexImage2D, (U, I, I, I, I, I, U, U, const void *));
    FN(void, TexSubImage2D, (U, I, I, I, I, I, U, U, const void *));
    FN(void, PixelStorei, (U, I));
    FN(void, GenFramebuffers, (I, U *));
    FN(void, BindFramebuffer, (U, U));
    FN(void, DeleteFramebuffers, (I, const U *));
    FN(void, FramebufferTexture2D, (U, U, U, U, I));
    FN(U, CheckFramebufferStatus, (U));
    FN(void, GenRenderbuffers, (I, U *));
    FN(void, BindRenderbuffer, (U, U));
    FN(void, DeleteRenderbuffers, (I, const U *));
    FN(void, RenderbufferStorage, (U, U, I, I));
    FN(void, FramebufferRenderbuffer, (U, U, U, U));
    FN(U, CreateShader, (U));
    FN(void, ShaderSource, (U, I, const char *const *, const I *));
    FN(void, CompileShader, (U));
    FN(void, GetShaderiv, (U, U, I *));
    FN(void, GetShaderInfoLog, (U, I, I *, char *));
    FN(void, DeleteShader, (U));
    FN(U, CreateProgram, ());
    FN(void, AttachShader, (U, U));
    FN(void, BindAttribLocation, (U, U, const char *));
    FN(void, LinkProgram, (U));
    FN(void, GetProgramiv, (U, U, I *));
    FN(void, GetProgramInfoLog, (U, I, I *, char *));
    FN(void, DeleteProgram, (U));
    FN(void, UseProgram, (U));
    FN(I, GetUniformLocation, (U, const char *));
    FN(void, Uniform1i, (I, I));
    FN(void, Uniform1f, (I, float));
    FN(void, UniformMatrix4fv, (I, I, B, const float *));
    FN(void, GenBuffers, (I, U *));
    FN(void, BindBuffer, (U, U));
    FN(void, DeleteBuffers, (I, const U *));
    FN(void, BufferData, (U, ptrdiff_t, const void *, U));
    FN(void, EnableVertexAttribArray, (U));
    FN(void, DisableVertexAttribArray, (U));
    FN(void, VertexAttribPointer, (U, I, U, B, I, const void *));
    FN(void, GetVertexAttribiv, (U, U, I *));
    FN(void, GetVertexAttribPointerv, (U, U, void **));
    FN(void, Viewport, (I, I, I, I));
    FN(void, Scissor, (I, I, I, I));
    FN(void, ColorMask, (B, B, B, B));
    FN(void, DepthMask, (B));
    FN(void, DepthFunc, (U));
    FN(void, ClearColor, (float, float, float, float));
    FN(void, Clear, (U));
    FN(void, ClearDepthf, (float));
    FN(void, ClearDepth, (double));
    FN(void, BlendFuncSeparate, (U, U, U, U));
    FN(void, BlendEquationSeparate, (U, U));
    FN(void, DrawArrays, (U, I, I));
    FN(void, CullFace, (U));
    FN(void, FrontFace, (U));
    FN(void, GetShaderPrecisionFormat, (U, U, I *, I *));
#undef FN
    bool es = false, rowLength = false, highp = true;
    template <typename T> void load(T &fn, const char *name, bool optional = false) {
        fn = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
        if (!fn) {
            std::string extension = std::string(name) + "EXT";
            fn = reinterpret_cast<T>(SDL_GL_GetProcAddress(extension.c_str()));
        }
        if (!fn && !optional)
            throw std::runtime_error(std::string("Missing volume GPU function: ") + name);
    }
    void load() {
#define L(N) load(N, "gl" #N)
        L(GetIntegerv);
        L(GetFloatv);
        L(GetBooleanv);
        L(GetString);
        L(IsEnabled);
        L(Enable);
        L(Disable);
        L(ActiveTexture);
        L(BindTexture);
        L(GenTextures);
        L(DeleteTextures);
        L(TexParameteri);
        L(TexImage2D);
        L(TexSubImage2D);
        L(PixelStorei);
        L(GenFramebuffers);
        L(BindFramebuffer);
        L(DeleteFramebuffers);
        L(FramebufferTexture2D);
        L(CheckFramebufferStatus);
        L(GenRenderbuffers);
        L(BindRenderbuffer);
        L(DeleteRenderbuffers);
        L(RenderbufferStorage);
        L(FramebufferRenderbuffer);
        L(CreateShader);
        L(ShaderSource);
        L(CompileShader);
        L(GetShaderiv);
        L(GetShaderInfoLog);
        L(DeleteShader);
        L(CreateProgram);
        L(AttachShader);
        L(BindAttribLocation);
        L(LinkProgram);
        L(GetProgramiv);
        L(GetProgramInfoLog);
        L(DeleteProgram);
        L(UseProgram);
        L(GetUniformLocation);
        L(Uniform1i);
        L(Uniform1f);
        L(UniformMatrix4fv);
        L(GenBuffers);
        L(BindBuffer);
        L(DeleteBuffers);
        L(BufferData);
        L(EnableVertexAttribArray);
        L(DisableVertexAttribArray);
        L(VertexAttribPointer);
        L(GetVertexAttribiv);
        L(GetVertexAttribPointerv);
        L(Viewport);
        L(Scissor);
        L(ColorMask);
        L(DepthMask);
        L(DepthFunc);
        L(ClearColor);
        L(Clear);
        L(BlendFuncSeparate);
        L(BlendEquationSeparate);
        L(DrawArrays);
        L(CullFace);
        L(FrontFace);
#undef L
        const auto version = reinterpret_cast<const char *>(GetString(0x1F02));
        es = version && std::strstr(version, "OpenGL ES");
        rowLength = !es || (version && std::strstr(version, "OpenGL ES 3"));
        if (es) {
            load(GetShaderPrecisionFormat, "glGetShaderPrecisionFormat");
            I range[2]{}, precision = 0;
            GetShaderPrecisionFormat(0x8B30, 0x8DF2, range, &precision);
            highp = precision > 0;
        }
        if (es)
            load(ClearDepthf, "glClearDepthf");
        else
            load(ClearDepth, "glClearDepth");
    }
};
// Preserve the state SDL owns, including each touched vertex attribute's VBO
// and pointer, texture unit zero, target, blending, depth and clipping state.
struct State {
    Gl &g;
    I program, buffer, fbo, rbo, active, texture, unpack, row = 0, viewport[4], scissor[4],
                                                          depthFunc, frontFace, cullMode;
    I srcRgb, dstRgb, srcAlpha, dstAlpha, equationRgb, equationAlpha;
    B depthWrite, colorWrite[4];
    float clearColor[4], clearDepth;
    B blendOn, depthOn, scissorOn, cullOn, textureOn = 0;
    struct Attribute {
        I enabled, size, type, normalized, stride, buffer;
        void *pointer = nullptr;
    };
    std::array<Attribute, 3> attributes;
    explicit State(Gl &gl) : g(gl) {
        g.GetIntegerv(0x8B8D, &program);
        g.GetIntegerv(0x8894, &buffer);
        g.GetIntegerv(0x8CA6, &fbo);
        g.GetIntegerv(0x8CA7, &rbo);
        g.GetIntegerv(0x84E0, &active);
        g.ActiveTexture(texture0);
        g.GetIntegerv(0x8069, &texture);
        g.GetIntegerv(0x0CF5, &unpack);
        if (g.rowLength)
            g.GetIntegerv(0x0CF2, &row);
        g.GetIntegerv(0x0BA2, viewport);
        g.GetIntegerv(0x0C10, scissor);
        g.GetIntegerv(0x0B74, &depthFunc);
        g.GetIntegerv(0x0B46, &frontFace);
        g.GetIntegerv(0x0B45, &cullMode);
        g.GetIntegerv(0x80C9, &srcRgb);
        g.GetIntegerv(0x80C8, &dstRgb);
        g.GetIntegerv(0x80CB, &srcAlpha);
        g.GetIntegerv(0x80CA, &dstAlpha);
        g.GetIntegerv(0x8009, &equationRgb);
        g.GetIntegerv(0x883D, &equationAlpha);
        g.GetBooleanv(0x0B72, &depthWrite);
        g.GetBooleanv(0x0C23, colorWrite);
        g.GetFloatv(0x0C22, clearColor);
        g.GetFloatv(0x0B73, &clearDepth);
        blendOn = g.IsEnabled(blend);
        depthOn = g.IsEnabled(depthTest);
        scissorOn = g.IsEnabled(scissorTest);
        cullOn = g.IsEnabled(cullFace);
        if (!g.es)
            textureOn = g.IsEnabled(tex2D);
        for (U i = 0; i < 3; ++i) {
            auto &a = attributes[i];
            g.GetVertexAttribiv(i, 0x8622, &a.enabled);
            g.GetVertexAttribiv(i, 0x8623, &a.size);
            g.GetVertexAttribiv(i, 0x8625, &a.type);
            g.GetVertexAttribiv(i, 0x886A, &a.normalized);
            g.GetVertexAttribiv(i, 0x8624, &a.stride);
            g.GetVertexAttribiv(i, 0x889F, &a.buffer);
            g.GetVertexAttribPointerv(i, 0x8645, &a.pointer);
        }
    }
    ~State() {
        for (U i = 0; i < 3; ++i) {
            const auto &a = attributes[i];
            g.BindBuffer(arrayBuffer, U(a.buffer));
            g.VertexAttribPointer(i, a.size, U(a.type), B(a.normalized), a.stride, a.pointer);
            if (a.enabled)
                g.EnableVertexAttribArray(i);
            else
                g.DisableVertexAttribArray(i);
        }
        g.BindBuffer(arrayBuffer, U(buffer));
        g.BindFramebuffer(framebuffer, U(fbo));
        g.BindRenderbuffer(renderbuffer, U(rbo));
        g.UseProgram(U(program));
        g.BindTexture(tex2D, U(texture));
        g.ActiveTexture(U(active));
        g.PixelStorei(0x0CF5, unpack);
        if (g.rowLength)
            g.PixelStorei(0x0CF2, row);
        g.Viewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        g.Scissor(scissor[0], scissor[1], scissor[2], scissor[3]);
        g.DepthMask(depthWrite);
        g.ColorMask(colorWrite[0], colorWrite[1], colorWrite[2], colorWrite[3]);
        g.DepthFunc(U(depthFunc));
        g.FrontFace(U(frontFace));
        g.CullFace(U(cullMode));
        g.BlendFuncSeparate(U(srcRgb), U(dstRgb), U(srcAlpha), U(dstAlpha));
        g.BlendEquationSeparate(U(equationRgb), U(equationAlpha));
        g.ClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        if (g.es)
            g.ClearDepthf(clearDepth);
        else
            g.ClearDepth(clearDepth);
        for (auto entry : {std::pair<U, B>{blend, blendOn},
                           {depthTest, depthOn},
                           {scissorTest, scissorOn},
                           {cullFace, cullOn}})
            if (entry.second)
                g.Enable(entry.first);
            else
                g.Disable(entry.first);
        if (!g.es) {
            if (textureOn)
                g.Enable(tex2D);
            else
                g.Disable(tex2D);
        }
    }
};
} // namespace
struct VoxelGpu::Impl {
    Gl g;
    SDL_GLContext context = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *picture = nullptr;
    U program = 0, atlas = 0, fbo = 0, depth = 0;
    I camera = -1, light = -1, image = -1;
    int width = 0, height = 0, atlasW = 0, atlasH = 0;
    uint64_t lastMesh = ~uint64_t(0);
    std::array<uint64_t, voxel::planeCount> atlasPixels{};
    std::array<int, 4> lastView{{-999, -999, -999, -999}};
    struct Buffer {
        U id = 0;
        I count = 0;
        uint64_t signature = ~uint64_t(0);
    };
    std::array<std::array<Buffer, 2>, voxel::planeCount> buffers;
    bool failed = false;
    int failedDetail = -1;
    const char *status = "voxelready";
    U shader(U type, const char *body) {
        std::string prefix = g.es ? "#version 100\nprecision highp float;\n" : "#version 120\n";
        if (g.es) {
            if (type == 0x8B30 && !g.highp)
                prefix = "#version 100\nprecision mediump float;\n";
            prefix += g.highp ? "#define SCU_VARYING highp\n" : "#define SCU_VARYING mediump\n";
        } else
            prefix += "#define SCU_VARYING\n";
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        prefix += "#define SCU_BIG_ENDIAN 1\n";
#endif
        const char *sources[] = {prefix.c_str(), body};
        U id = g.CreateShader(type);
        g.ShaderSource(id, 2, sources, nullptr);
        g.CompileShader(id);
        I ok = 0;
        g.GetShaderiv(id, 0x8B81, &ok);
        if (!ok) {
            char text[1024]{};
            g.GetShaderInfoLog(id, 1024, nullptr, text);
            g.DeleteShader(id);
            throw std::runtime_error(text);
        }
        return id;
    }
    void initialise(SDL_Renderer *r) {
        context = SDL_GL_GetCurrentContext();
        renderer = r;
        if (!context)
            throw std::runtime_error("A compatible OpenGL renderer is required for pixel volumes");
        g.load();
        State state(g);
        U vs = shader(0x8B31, voxel::volumeVertex), fs = 0;
        try {
            fs = shader(0x8B30, voxel::volumeFragment);
        } catch (...) {
            g.DeleteShader(vs);
            throw;
        }
        program = g.CreateProgram();
        g.AttachShader(program, vs);
        g.AttachShader(program, fs);
        g.BindAttribLocation(program, 0, "aPosition");
        g.BindAttribLocation(program, 1, "aNormal");
        g.BindAttribLocation(program, 2, "aTexcoord");
        g.LinkProgram(program);
        g.DeleteShader(vs);
        g.DeleteShader(fs);
        I ok = 0;
        g.GetProgramiv(program, 0x8B82, &ok);
        if (!ok) {
            char text[1024]{};
            g.GetProgramInfoLog(program, 1024, nullptr, text);
            throw std::runtime_error(text);
        }
        camera = g.GetUniformLocation(program, "uCamera");
        light = g.GetUniformLocation(program, "uLight");
        image = g.GetUniformLocation(program, "uImage");
        g.GenTextures(1, &atlas);
        g.GenFramebuffers(1, &fbo);
        g.GenRenderbuffers(1, &depth);
    }
    void release(bool gpu = true) {
        if (gpu && context && context == SDL_GL_GetCurrentContext() && g.DeleteProgram) {
            for (auto &layer : buffers)
                for (auto &b : layer)
                    if (b.id)
                        g.DeleteBuffers(1, &b.id);
            if (atlas)
                g.DeleteTextures(1, &atlas);
            if (program)
                g.DeleteProgram(program);
            if (fbo)
                g.DeleteFramebuffers(1, &fbo);
            if (depth)
                g.DeleteRenderbuffers(1, &depth);
        }
        if (picture)
            SDL_DestroyTexture(picture);
        picture = nullptr;
        context = nullptr;
        program = atlas = fbo = depth = 0;
        for (auto &layer : buffers)
            for (auto &b : layer)
                b = {};
    }
    ~Impl() {
        release();
    }
    void resize(int w, int h) {
        if (picture && w == width && h == height)
            return;
        if (picture)
            SDL_DestroyTexture(picture);
        // RGBA32 matches GL's logical render-target channels on both endian
        // variants; SDL can perform any required target-format conversion.
        picture =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, w, h);
        if (!picture)
            throw std::runtime_error(SDL_GetError());
        auto premul = SDL_ComposeCustomBlendMode(
            SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
            SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
        if (SDL_SetTextureBlendMode(picture, premul))
            throw std::runtime_error(SDL_GetError());
        SDL_SetTextureScaleMode(picture, SDL_ScaleModeLinear);
        I active = 0;
        g.GetIntegerv(0x84E0, &active);
        g.ActiveTexture(texture0);
        float tw = 0, th = 0;
        const bool bound = SDL_GL_BindTexture(picture, &tw, &th) == 0;
        I output = 0;
        if (bound) {
            g.GetIntegerv(0x8069, &output);
            SDL_GL_UnbindTexture(picture);
        }
        g.ActiveTexture(U(active));
        if (!bound || std::abs(tw - 1.f) > .001f || std::abs(th - 1.f) > .001f)
            throw std::runtime_error("Volume output needs a standard 2D texture");
        // Bind/Unbind updates SDL's own texture cache. Snapshot afterwards so
        // restoring GL does not contradict that cache on the next SDL command.
        State state(g);
        g.BindFramebuffer(framebuffer, fbo);
        g.FramebufferTexture2D(framebuffer, 0x8CE0, tex2D, U(output), 0);
        g.BindRenderbuffer(renderbuffer, depth);
        g.RenderbufferStorage(renderbuffer, 0x81A5, w, h);
        g.FramebufferRenderbuffer(framebuffer, 0x8D00, renderbuffer, depth);
        if (g.CheckFramebufferStatus(framebuffer) != 0x8CD5)
            throw std::runtime_error("Volume framebuffer is incomplete");
        width = w;
        height = h;
        lastMesh = ~uint64_t(0);
    }
    bool atlasDirty(const voxel::Frame &f, const voxel::Mesher &mesh) const {
        if (atlasW != f.textureWidth() || atlasH != f.textureHeight())
            return true;
        for (size_t p = 0; p < voxel::planeCount; ++p)
            if (atlasPixels[p] != mesh.layers()[p].pixelSignature)
                return true;
        return false;
    }
    void upload(const voxel::Frame &f, const voxel::Mesher &mesh, bool texelsChanged) {
        if (texelsChanged) {
            g.BindTexture(tex2D, atlas);
            g.PixelStorei(0x0CF5, 4);
            if (g.rowLength)
                g.PixelStorei(0x0CF2, 0);
            g.TexParameteri(tex2D, 0x2801, 0x2600);
            g.TexParameteri(tex2D, 0x2800, 0x2600);
            g.TexParameteri(tex2D, 0x2802, 0x812F);
            g.TexParameteri(tex2D, 0x2803, 0x812F);
            if (atlasW != f.textureWidth() || atlasH != f.textureHeight()) {
                I maximum = 0;
                g.GetIntegerv(0x0D33, &maximum);
                if (f.textureWidth() > maximum || f.textureHeight() > maximum)
                    throw std::runtime_error("Lower volume detail for this GPU's texture limit");
                g.TexImage2D(tex2D, 0, I(rgba), f.textureWidth(), f.textureHeight(), 0, rgba, ubyte,
                             f.pixels.data());
                atlasW = f.textureWidth();
                atlasH = f.textureHeight();
            } else if (g.rowLength) {
                g.PixelStorei(0x0CF2, atlasW);
                for (size_t p = 0; p < voxel::planeCount; ++p) {
                    if (atlasPixels[p] == mesh.layers()[p].pixelSignature)
                        continue;
                    int x = int(p % 2) * f.width, y = int(p / 2) * f.height;
                    g.TexSubImage2D(tex2D, 0, x, y, f.width, f.height, rgba, ubyte,
                                    f.pixels.data() + size_t(y) * atlasW + x);
                }
                g.PixelStorei(0x0CF2, 0);
            } else
                g.TexSubImage2D(tex2D, 0, 0, 0, f.textureWidth(), f.textureHeight(), rgba, ubyte,
                                f.pixels.data());
            for (size_t p = 0; p < voxel::planeCount; ++p)
                atlasPixels[p] = mesh.layers()[p].pixelSignature;
        }
        for (size_t i = 0; i < voxel::planeCount; ++i)
            for (size_t alpha = 0; alpha < 2; ++alpha) {
                const auto &layer = mesh.layers()[i];
                auto &b = buffers[i][alpha];
                if (b.signature == layer.signature)
                    continue;
                const auto &vertices = alpha ? layer.translucent : layer.opaque;
                if (!b.id)
                    g.GenBuffers(1, &b.id);
                g.BindBuffer(arrayBuffer, b.id);
                g.BufferData(arrayBuffer, ptrdiff_t(vertices.size() * sizeof(voxel::Vertex)),
                             vertices.data(), 0x88E4);
                b.count = I(vertices.size());
                b.signature = layer.signature;
            }
    }
    SDL_Texture *render(const voxel::Frame &f, const voxel::Mesher &mesh, const Settings &s, int w,
                        int h) {
        // Quantized quality changes avoid reallocating an image on each small
        // governor adjustment. The selected camera extent never changes.
        int cap = s.wearActive ? 512 : s.quality == 0 ? 854 : s.quality == 2 ? 1920 : 1280;
        float limit = std::min({1.f, float(cap) / std::max(1, w), 1080.f / std::max(1, h)});
        int scale = s.autoPerformance ? s.compositionScale : 100;
        float bucket = scale > 85 ? 1.f : scale > 65 ? .75f : scale > 42 ? .5f : .35f;
        w = std::max(8, int(w * limit * bucket) / 8 * 8);
        h = std::max(8, int(h * limit * bucket) / 8 * 8);
        std::array<int, 4> view{{s.voxelYaw, s.voxelPitch, s.voxelLight, w * 8192 + h}};
        bool texelsChanged = atlasDirty(f, mesh);
        if (picture && !texelsChanged && lastMesh == mesh.revision && lastView == view)
            return picture;
        SDL_RenderFlush(renderer);
        resize(w, h);
        State state(g);
        upload(f, mesh, texelsChanged);
        g.BindFramebuffer(framebuffer, fbo);
        g.Viewport(0, 0, w, h);
        g.Disable(scissorTest);
        g.Enable(cullFace);
        g.CullFace(0x0405);
        g.FrontFace(0x0901);
        g.Enable(depthTest);
        g.DepthFunc(0x0203);
        g.DepthMask(1);
        g.ColorMask(1, 1, 1, 1);
        g.Disable(blend);
        g.ClearColor(0, 0, 0, 0);
        if (g.es)
            g.ClearDepthf(1);
        else
            g.ClearDepth(1);
        g.Clear(0x00004000 | 0x00000100);
        g.UseProgram(program);
        g.Uniform1i(image, 0);
        g.Uniform1f(light, s.voxelLight * .01f);
        auto matrix =
            voxel::pictureCamera(mesh.bounds(), w, h, float(s.voxelYaw), float(s.voxelPitch));
        // SDL target textures address row zero at the top. Render that row at
        // the GL image's bottom so ordinary SDL copies need no special flip.
        for (int column = 0; column < 4; ++column)
            matrix.v[column * 4 + 1] *= -1;
        g.UniformMatrix4fv(camera, 1, 0, matrix.v.data());
        g.BindTexture(tex2D, atlas);
        for (U i = 0; i < 3; ++i)
            g.EnableVertexAttribArray(i);
        for (size_t alpha = 0; alpha < 2; ++alpha) {
            if (alpha) {
                g.Enable(blend);
                g.BlendFuncSeparate(1, 0x0303, 1, 0x0303);
                g.BlendEquationSeparate(0x8006, 0x8006);
                g.DepthMask(0);
            }
            for (auto &layer : buffers) {
                auto &b = layer[alpha];
                if (!b.count)
                    continue;
                g.BindBuffer(arrayBuffer, b.id);
                g.VertexAttribPointer(
                    0, 3, floatType, 0, sizeof(voxel::Vertex),
                    reinterpret_cast<const void *>(offsetof(voxel::Vertex, position)));
                g.VertexAttribPointer(
                    1, 3, floatType, 0, sizeof(voxel::Vertex),
                    reinterpret_cast<const void *>(offsetof(voxel::Vertex, normal)));
                g.VertexAttribPointer(2, 2, floatType, 0, sizeof(voxel::Vertex),
                                      reinterpret_cast<const void *>(offsetof(voxel::Vertex, u)));
                g.DrawArrays(triangles, 0, b.count);
            }
        }
        lastMesh = mesh.revision;
        lastView = view;
        status = "voxelready";
        return picture;
    }
};
VoxelGpu::VoxelGpu() : impl(new Impl) {}
VoxelGpu::~VoxelGpu() = default;
SDL_Texture *VoxelGpu::render(SDL_Renderer *r, const voxel::Frame &f, const voxel::Mesher &mesh,
                              const Settings &s, int w, int h) {
    if (!f.valid() || !mesh.valid() || !s.voxelMode)
        return nullptr;
    if (impl->failed) {
        if (impl->failedDetail != s.voxelDetail || impl->context != SDL_GL_GetCurrentContext())
            release();
        else
            return nullptr;
    }
    try {
        if (!impl->program)
            impl->initialise(r);
        return impl->render(f, mesh, s, w, h);
    } catch (const std::exception &error) {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "SCU pixel volume fallback: %s", error.what());
        impl->failed = true;
        impl->failedDetail = s.voxelDetail;
        impl->status = "voxelunsupported";
        return nullptr;
    }
}
void VoxelGpu::release() {
    impl.reset(new Impl);
}
void VoxelGpu::invalidate() {
    impl->release(false);
    impl.reset(new Impl);
}
const char *VoxelGpu::status() const {
    return impl->status;
}
} // namespace scu
