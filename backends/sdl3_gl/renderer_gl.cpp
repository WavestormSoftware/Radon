// Copyright (c) 2026 Wavestorm Software
// Draws a Radon draw list with OpenGL 3.3 core or OpenGL ES 3 / WebGL2. GL entry points are loaded
// through SDL so no GL headers or loader library are needed.
#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <cstdio>
#include <string>
#include <vector>

#include "radon_sdl3_gl.hpp"
#include "shaders.hpp"

namespace rd::gl {
namespace {

#ifdef _WIN32
#define RD_GLCALL __stdcall
#else
#define RD_GLCALL
#endif

using GLenum = unsigned int;
using GLuint = unsigned int;
using GLint = int;
using GLsizei = int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLfloat = float;
using GLsizeiptr = std::ptrdiff_t;
using GLchar = char;

enum : GLenum {
    GL_TRIANGLES = 0x0004,
    GL_UNSIGNED_INT = 0x1405,
    GL_FLOAT = 0x1406,
    GL_UNSIGNED_BYTE = 0x1401,
    GL_ARRAY_BUFFER = 0x8892,
    GL_ELEMENT_ARRAY_BUFFER = 0x8893,
    GL_STREAM_DRAW = 0x88E0,
    GL_VERTEX_SHADER = 0x8B31,
    GL_FRAGMENT_SHADER = 0x8B30,
    GL_COMPILE_STATUS = 0x8B81,
    GL_LINK_STATUS = 0x8B82,
    GL_INFO_LOG_LENGTH = 0x8B84,
    GL_TEXTURE_2D = 0x0DE1,
    GL_TEXTURE0 = 0x84C0,
    GL_TEXTURE_MIN_FILTER = 0x2801,
    GL_TEXTURE_MAG_FILTER = 0x2800,
    GL_LINEAR = 0x2601,
    GL_TEXTURE_WRAP_S = 0x2802,
    GL_TEXTURE_WRAP_T = 0x2803,
    GL_CLAMP_TO_EDGE = 0x812F,
    GL_R8 = 0x8229,
    GL_RED = 0x1903,
    GL_RGBA = 0x1908,
    GL_RGBA8 = 0x8058,
    GL_BLEND = 0x0BE2,
    GL_SRC_ALPHA = 0x0302,
    GL_ONE_MINUS_SRC_ALPHA = 0x0303,
    GL_ONE = 1,
    GL_SCISSOR_TEST = 0x0C11,
    GL_DEPTH_TEST = 0x0B71,
    GL_CULL_FACE = 0x0B44,
    GL_COLOR_BUFFER_BIT = 0x4000,
    GL_UNPACK_ALIGNMENT = 0x0CF5,
    GL_PACK_ALIGNMENT = 0x0D05,
    GL_FALSE_ = 0,
    GL_FRAMEBUFFER = 0x8D40,
    GL_RENDERBUFFER = 0x8D41,
    GL_COLOR_ATTACHMENT0 = 0x8CE0,
    GL_DEPTH_ATTACHMENT = 0x8D00,
    GL_DEPTH_COMPONENT24 = 0x81A6,
    GL_FRAMEBUFFER_COMPLETE = 0x8CD5,
    GL_FRAMEBUFFER_BINDING = 0x8CA6,
    GL_VIEWPORT = 0x0BA2,
};

// clang-format off
#define RD_GL_FUNCS(X)                                                                                       \
    X(GLuint, CreateShader, (GLenum))                                                                        \
    X(void, ShaderSource, (GLuint, GLsizei, const GLchar* const*, const GLint*)) X(                          \
        void, CompileShader, (GLuint)) X(void, GetShaderiv, (GLuint, GLenum, GLint*))                        \
        X(void, GetShaderInfoLog, (GLuint, GLsizei, GLsizei*, GLchar*)) X(GLuint, CreateProgram, ()) X(      \
            void, AttachShader, (GLuint, GLuint)) X(void, LinkProgram, (GLuint)) X(void, GetProgramiv,       \
                                                                                   (GLuint, GLenum, GLint*)) \
            X(void, GetProgramInfoLog, (GLuint, GLsizei, GLsizei*, GLchar*)) X(void, DeleteShader, (GLuint)) \
                X(void, DeleteProgram, (GLuint)) X(void, UseProgram, (GLuint)) X(                            \
                    GLint, GetUniformLocation,                                                               \
                    (GLuint, const GLchar*)) X(void, Uniform2f, (GLint, GLfloat, GLfloat))                   \
                    X(void, Uniform1i, (GLint, GLint)) X(void, GenBuffers, (GLsizei, GLuint*)) X(            \
                        void,                                                                                \
                        BindBuffer,                                                                          \
                        (GLenum, GLuint)) X(void, BufferData, (GLenum, GLsizeiptr, const void*, GLenum))     \
                        X(void, DeleteBuffers, (GLsizei, const GLuint*)) X(                                  \
                            void,                                                                            \
                            GenVertexArrays, (GLsizei, GLuint*)) X(void, BindVertexArray, (GLuint))          \
                            X(void, DeleteVertexArrays,                                                      \
                              (GLsizei, const GLuint*)) X(void, EnableVertexAttribArray, (GLuint))           \
                                X(void, VertexAttribPointer,                                                 \
                                  (GLuint, GLint, GLenum, GLboolean, GLsizei,                                \
                                   const void*)) X(void, GenTextures, (GLsizei, GLuint*))                    \
                                    X(void, BindTexture, (GLenum, GLuint)) X(                                \
                                        void,                                                                \
                                        TexImage2D,                                                          \
                                        (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum,      \
                                         const void*))                                                       \
                                        X(void, TexSubImage2D,                                               \
                                          (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum,    \
                                           const void*)) X(void, TexParameteri, (GLenum, GLenum, GLint))     \
                                            X(void, DeleteTextures, (GLsizei, const GLuint*)) X(             \
                                                void,                                                        \
                                                ActiveTexture,                                               \
                                                (GLenum)) X(void, PixelStorei, (GLenum, GLint))              \
                                                X(void, Enable, (GLenum)) X(void, Disable, (GLenum)) X(      \
                                                    void,                                                    \
                                                    BlendFuncSeparate, (GLenum, GLenum, GLenum, GLenum))     \
                                                    X(void, Viewport, (GLint, GLint, GLsizei, GLsizei))      \
                                                        X(void, Scissor, (GLint, GLint, GLsizei, GLsizei))   \
                                                            X(void, DrawElements,                            \
                                                              (GLenum, GLsizei, GLenum, const void*))        \
                                                                X(void, ClearColor,                          \
                                                                  (GLfloat, GLfloat, GLfloat, GLfloat))      \
                                                                    X(void, Clear, (GLbitfield))             \
                                                                        X(void, ReadPixels,                  \
                                                                          (GLint, GLint, GLsizei, GLsizei,   \
                                                                           GLenum, GLenum, void*))
// clang-format on
// clang-format off
#define RD_GL_FBO_FUNCS(X)                                                                            \
    X(void, GenFramebuffers, (GLsizei, GLuint*))                                                      \
    X(void, BindFramebuffer, (GLenum, GLuint)) X(void, DeleteFramebuffers, (GLsizei, const GLuint*))  \
        X(void, FramebufferTexture2D, (GLenum, GLenum, GLenum, GLuint, GLint))                        \
            X(GLenum, CheckFramebufferStatus, (GLenum)) X(void, GenRenderbuffers, (GLsizei, GLuint*)) \
                X(void, BindRenderbuffer, (GLenum, GLuint))                                           \
                    X(void, DeleteRenderbuffers, (GLsizei, const GLuint*))                            \
                        X(void, RenderbufferStorage, (GLenum, GLenum, GLsizei, GLsizei))              \
                            X(void, FramebufferRenderbuffer, (GLenum, GLenum, GLenum, GLuint))        \
                                X(void, GetIntegerv, (GLenum, GLint*))
// clang-format on

#define RD_DECL(ret, name, args) ret(RD_GLCALL* name) args = nullptr;
}  // namespace

struct Renderer {
    RD_GL_FUNCS(RD_DECL)
    RD_GL_FBO_FUNCS(RD_DECL)
    GLuint program = 0, vao = 0, vbo = 0, ebo = 0, atlas_tex = 0;
    GLint u_scale = -1, u_tex = -1;
    uint32_t atlas_version = 0;
    int atlas_size = 0;
    std::vector<GLuint> textures;  // TextureId n (>= 1) is textures[n - 1]; 0 = destroyed
    std::vector<uint8_t> owned;    // 1 if Radon created (and must delete) the GL texture
    struct Target {
        TextureId tex = 0;
        GLuint fbo = 0, depth = 0;
        int w = 0, h = 0;
    };
    std::vector<Target> targets;
    GLint saved_fbo = 0, saved_vp[4] = {};
    bool in_target = false;
};

namespace {

bool load_functions(Renderer* r) {
#define RD_LOAD(ret, name, args)                                                         \
    r->name = reinterpret_cast<ret(RD_GLCALL*) args>(SDL_GL_GetProcAddress("gl" #name)); \
    if (!r->name) {                                                                      \
        std::fprintf(stderr, "radon: missing GL function gl%s\n", #name);                \
        return false;                                                                    \
    }
    RD_GL_FUNCS(RD_LOAD)
    RD_GL_FBO_FUNCS(RD_LOAD)
#undef RD_LOAD
    return true;
}

GLuint compile(Renderer* r, GLenum type, const std::string& src) {
    GLuint sh = r->CreateShader(type);
    const char* p = src.c_str();
    r->ShaderSource(sh, 1, &p, nullptr);
    r->CompileShader(sh);
    GLint ok = 0;
    r->GetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        r->GetShaderInfoLog(sh, sizeof log, nullptr, log);
        std::fprintf(stderr, "radon: shader compile failed:\n%s\n", log);
        r->DeleteShader(sh);
        return 0;
    }
    return sh;
}

void set_tex_params(Renderer* r) {
    r->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    r->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    r->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    r->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// Registers a GL texture under the lowest free TextureId (destroyed ids are reused, so creating and
// destroying views at runtime does not grow the table).
TextureId alloc_slot(Renderer* r, GLuint gl, bool owned) {
    for (size_t i = 0; i < r->textures.size(); i++)
        if (r->textures[i] == 0) {
            r->textures[i] = gl;
            r->owned[i] = owned;
            return (TextureId)(i + 1);
        }
    r->textures.push_back(gl);
    r->owned.push_back(owned);
    return (TextureId)r->textures.size();
}

void delete_target_gl(Renderer* r, Renderer::Target& t) {
    if (t.fbo) r->DeleteFramebuffers(1, &t.fbo);
    if (t.depth) r->DeleteRenderbuffers(1, &t.depth);
    t.fbo = t.depth = 0;
}

}  // namespace

Renderer* create_renderer(const RendererConfig& cfg) {
    std::unique_ptr<Renderer> r(new Renderer);
    if (!load_functions(r.get())) return nullptr;
    const std::string header = cfg.gles
                                   ? "#version 300 es\nprecision highp float;\nprecision highp sampler2D;\n"
                                   : "#version 330 core\n";
    GLuint vs = compile(r.get(), GL_VERTEX_SHADER, header + kVertexBody);
    GLuint fs = compile(r.get(), GL_FRAGMENT_SHADER, header + kFragmentBody);
    if (!vs || !fs) {
        if (vs) r->DeleteShader(vs);
        if (fs) r->DeleteShader(fs);
        return nullptr;
    }
    r->program = r->CreateProgram();
    r->AttachShader(r->program, vs);
    r->AttachShader(r->program, fs);
    r->LinkProgram(r->program);
    r->DeleteShader(vs);
    r->DeleteShader(fs);
    GLint ok = 0;
    r->GetProgramiv(r->program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        r->GetProgramInfoLog(r->program, sizeof log, nullptr, log);
        std::fprintf(stderr, "radon: program link failed:\n%s\n", log);
        r->DeleteProgram(r->program);
        return nullptr;
    }
    r->u_scale = r->GetUniformLocation(r->program, "u_scale");
    r->u_tex = r->GetUniformLocation(r->program, "u_tex");

    r->GenVertexArrays(1, &r->vao);
    r->BindVertexArray(r->vao);
    r->GenBuffers(1, &r->vbo);
    r->GenBuffers(1, &r->ebo);
    r->BindBuffer(GL_ARRAY_BUFFER, r->vbo);
    r->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ebo);
    const GLsizei stride = (GLsizei)sizeof(DrawVertex);
    r->EnableVertexAttribArray(0);
    r->VertexAttribPointer(0, 2, GL_FLOAT, 0, stride, (const void*)offsetof(DrawVertex, pos));
    r->EnableVertexAttribArray(1);
    r->VertexAttribPointer(1, 2, GL_FLOAT, 0, stride, (const void*)offsetof(DrawVertex, uv));
    r->EnableVertexAttribArray(2);
    r->VertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, 1, stride, (const void*)offsetof(DrawVertex, color));
    r->EnableVertexAttribArray(3);
    r->VertexAttribPointer(3, 2, GL_FLOAT, 0, stride, (const void*)offsetof(DrawVertex, local));
    r->EnableVertexAttribArray(4);
    r->VertexAttribPointer(4, 4, GL_FLOAT, 0, stride, (const void*)offsetof(DrawVertex, shape));
    r->GenTextures(1, &r->atlas_tex);
    return r.release();
}

void destroy_renderer(Renderer* r) {
    if (!r) return;
    r->DeleteProgram(r->program);
    r->DeleteVertexArrays(1, &r->vao);
    r->DeleteBuffers(1, &r->vbo);
    r->DeleteBuffers(1, &r->ebo);
    r->DeleteTextures(1, &r->atlas_tex);
    for (auto& t : r->targets) delete_target_gl(r, t);
    for (size_t i = 0; i < r->textures.size(); i++)
        if (r->owned[i] && r->textures[i]) r->DeleteTextures(1, &r->textures[i]);
    delete r;
}

TextureId create_texture(Renderer* r, int w, int h, const uint8_t* rgba) {
    if (!r || w <= 0 || h <= 0 || !rgba) return 0;
    GLuint t = 0;
    r->GenTextures(1, &t);
    r->BindTexture(GL_TEXTURE_2D, t);
    r->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    r->TexImage2D(GL_TEXTURE_2D, 0, (GLint)GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    set_tex_params(r);
    return alloc_slot(r, t, true);
}

TextureId adopt_texture(Renderer* r, unsigned gl_texture) {
    if (!r || !gl_texture) return 0;
    return alloc_slot(r, gl_texture, false);
}

bool update_texture(Renderer* r, TextureId id, int w, int h, const uint8_t* rgba) {
    if (!r || id == 0 || id > r->textures.size() || !r->textures[id - 1] || !r->owned[id - 1] || w <= 0 ||
        h <= 0 || !rgba)
        return false;
    GLint prev = 0;
    r->GetIntegerv(0x8069 /*GL_TEXTURE_BINDING_2D*/, &prev);
    r->BindTexture(GL_TEXTURE_2D, r->textures[id - 1]);
    r->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    r->TexImage2D(GL_TEXTURE_2D, 0, (GLint)GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    r->BindTexture(GL_TEXTURE_2D, (GLuint)prev);
    return true;
}

void destroy_texture(Renderer* r, TextureId id) {
    if (!r || id == 0 || id > r->textures.size()) return;
    GLuint& t = r->textures[id - 1];
    for (auto& tg : r->targets)
        if (tg.tex == id) delete_target_gl(r, tg);
    r->targets.erase(std::remove_if(r->targets.begin(), r->targets.end(),
                                    [id](const Renderer::Target& tg) { return tg.tex == id; }),
                     r->targets.end());
    if (t && r->owned[id - 1]) r->DeleteTextures(1, &t);
    t = 0;
}

namespace {
// (Re)allocates the color texture and depth buffer of a target at w x h and attaches them.
bool alloc_target(Renderer* r, Renderer::Target& tg, int w, int h, bool depth) {
    GLint prev_tex = 0, prev_fbo = 0;
    r->GetIntegerv(0x8069, &prev_tex);
    r->GetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    r->BindTexture(GL_TEXTURE_2D, r->textures[tg.tex - 1]);
    r->TexImage2D(GL_TEXTURE_2D, 0, (GLint)GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    set_tex_params(r);
    if (!tg.fbo) r->GenFramebuffers(1, &tg.fbo);
    r->BindFramebuffer(GL_FRAMEBUFFER, tg.fbo);
    r->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->textures[tg.tex - 1], 0);
    if (depth) {
        if (!tg.depth) r->GenRenderbuffers(1, &tg.depth);
        r->BindRenderbuffer(GL_RENDERBUFFER, tg.depth);
        r->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        r->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tg.depth);
    }
    const bool ok = r->CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    r->BindFramebuffer(GL_FRAMEBUFFER, (GLuint)prev_fbo);
    r->BindTexture(GL_TEXTURE_2D, (GLuint)prev_tex);
    tg.w = w;
    tg.h = h;
    return ok;
}
}  // namespace

TextureId create_render_target(Renderer* r, int w, int h, bool depth) {
    if (!r || w <= 0 || h <= 0) return 0;
    GLuint t = 0;
    r->GenTextures(1, &t);
    Renderer::Target tg;
    tg.tex = alloc_slot(r, t, true);
    if (!alloc_target(r, tg, w, h, depth)) {
        delete_target_gl(r, tg);
        r->DeleteTextures(1, &t);
        r->textures[tg.tex - 1] = 0;
        return 0;
    }
    r->targets.push_back(tg);
    return tg.tex;
}

bool resize_render_target(Renderer* r, TextureId id, int w, int h) {
    if (!r || w <= 0 || h <= 0 || r->in_target) return false;
    for (auto& tg : r->targets)
        if (tg.tex == id) return tg.w == w && tg.h == h ? true : alloc_target(r, tg, w, h, tg.depth != 0);
    return false;
}

bool begin_target(Renderer* r, TextureId id, Color clear_color) {
    if (!r || r->in_target) return false;
    for (auto& tg : r->targets) {
        if (tg.tex != id || !tg.fbo) continue;
        r->GetIntegerv(GL_FRAMEBUFFER_BINDING, &r->saved_fbo);
        r->GetIntegerv(GL_VIEWPORT, r->saved_vp);
        r->BindFramebuffer(GL_FRAMEBUFFER, tg.fbo);
        r->Viewport(0, 0, tg.w, tg.h);
        r->ClearColor(clear_color.r / 255.0f, clear_color.g / 255.0f, clear_color.b / 255.0f,
                      clear_color.a / 255.0f);
        r->Clear(GL_COLOR_BUFFER_BIT | (tg.depth ? 0x100u /*GL_DEPTH_BUFFER_BIT*/ : 0u));
        r->in_target = true;
        return true;
    }
    return false;
}

void end_target(Renderer* r) {
    if (!r || !r->in_target) return;
    r->BindFramebuffer(GL_FRAMEBUFFER, (GLuint)r->saved_fbo);
    r->Viewport(r->saved_vp[0], r->saved_vp[1], r->saved_vp[2], r->saved_vp[3]);
    r->in_target = false;
}

void render(Renderer* r, const Context& ctx, int fb_w, int fb_h) {
    if (!r || fb_w <= 0 || fb_h <= 0) return;
    const DrawList& dl = ctx.draw_list();
    const float dpi = ctx.dpi_scale();

    r->Viewport(0, 0, fb_w, fb_h);
    r->Disable(GL_DEPTH_TEST);
    r->Disable(GL_CULL_FACE);
    r->Enable(GL_BLEND);
    r->BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    r->Enable(GL_SCISSOR_TEST);
    r->UseProgram(r->program);
    r->Uniform2f(r->u_scale, 2.0f * dpi / (float)fb_w, 2.0f * dpi / (float)fb_h);
    r->Uniform1i(r->u_tex, 0);
    r->ActiveTexture(GL_TEXTURE0);

    r->BindTexture(GL_TEXTURE_2D, r->atlas_tex);
    if (r->atlas_version != ctx.atlas_version()) {
        r->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
        const int n = ctx.atlas_size();
        if (r->atlas_size != n) {
            r->TexImage2D(GL_TEXTURE_2D, 0, (GLint)GL_R8, n, n, 0, GL_RED, GL_UNSIGNED_BYTE,
                          ctx.atlas_pixels());
            set_tex_params(r);
            r->atlas_size = n;
        } else {
            r->TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, n, n, GL_RED, GL_UNSIGNED_BYTE, ctx.atlas_pixels());
        }
        r->atlas_version = ctx.atlas_version();
    }

    r->BindVertexArray(r->vao);
    r->BindBuffer(GL_ARRAY_BUFFER, r->vbo);
    r->BufferData(GL_ARRAY_BUFFER, (GLsizeiptr)dl.vertex_count() * (GLsizeiptr)sizeof(DrawVertex),
                  dl.vertices(), GL_STREAM_DRAW);
    r->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ebo);
    r->BufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)dl.index_count() * (GLsizeiptr)sizeof(uint32_t),
                  dl.indices(), GL_STREAM_DRAW);

    GLuint bound = r->atlas_tex;
    for (int i = 0; i < dl.command_count(); i++) {
        const DrawCommand& c = dl.commands()[i];
        float x0 = std::fmax(0.0f, c.clip.x * dpi), y0 = std::fmax(0.0f, c.clip.y * dpi);
        float x1 = std::fmin((float)fb_w, (c.clip.x + c.clip.w) * dpi);
        float y1 = std::fmin((float)fb_h, (c.clip.y + c.clip.h) * dpi);
        if (x1 <= x0 || y1 <= y0) continue;
        r->Scissor((GLint)x0, (GLint)((float)fb_h - y1), (GLsizei)(x1 - x0 + 0.5f),
                   (GLsizei)(y1 - y0 + 0.5f));
        GLuint want = c.texture == 0 ? r->atlas_tex
                                     : (c.texture <= r->textures.size() && r->textures[c.texture - 1]
                                            ? r->textures[c.texture - 1]
                                            : r->atlas_tex);
        if (want != bound) {
            r->BindTexture(GL_TEXTURE_2D, want);
            bound = want;
        }
        r->DrawElements(GL_TRIANGLES, (GLsizei)c.index_count, GL_UNSIGNED_INT,
                        (const void*)((uintptr_t)c.index_offset * sizeof(uint32_t)));
    }
    r->Disable(GL_SCISSOR_TEST);
}

void clear(Renderer* r, Color c) {
    if (!r) return;
    r->ClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
    r->Clear(GL_COLOR_BUFFER_BIT);
}

bool read_pixels(Renderer* r, int w, int h, uint8_t* out) {
    if (!r || w <= 0 || h <= 0 || !out) return false;
    r->PixelStorei(GL_PACK_ALIGNMENT, 1);
    r->ReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, out);
    const size_t row = (size_t)w * 4;
    std::vector<uint8_t> tmp(row);
    for (int y = 0; y < h / 2; y++) {  // GL is bottom-up
        uint8_t* a = out + (size_t)y * row;
        uint8_t* b = out + (size_t)(h - 1 - y) * row;
        std::memcpy(tmp.data(), a, row);
        std::memcpy(a, b, row);
        std::memcpy(b, tmp.data(), row);
    }
    return true;
}

}  // namespace rd::gl
