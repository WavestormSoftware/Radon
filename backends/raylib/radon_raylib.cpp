// Copyright (c) 2026 Wavestorm Software
#include "radon_raylib.hpp"

#include <rlgl.h>

#include <cstring>

namespace rd::rl {

namespace {
const char* const kVs = R"(#version 330
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_local;   // DrawVertex::local in xy
layout(location = 3) in vec4 a_color;
layout(location = 4) in vec4 a_shape;
uniform vec2 u_scale;
out vec2 v_uv; out vec4 v_color; out vec2 v_local; out vec4 v_shape;
void main() {
    v_uv = a_uv; v_color = a_color; v_local = a_local.xy; v_shape = a_shape;
    gl_Position = vec4(a_pos.x * u_scale.x - 1.0, 1.0 - a_pos.y * u_scale.y, 0.0, 1.0);
}
)";
// Same math as backends/sdl3_gl/shaders.cpp.
const char* const kFs = R"(#version 330
uniform sampler2D u_tex;
in vec2 v_uv; in vec4 v_color; in vec2 v_local; in vec4 v_shape;
out vec4 o_color;
float sd_box(vec2 p, vec2 h, float r) { vec2 q = abs(p) - h + r; return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r; }
void main() {
    float mode = v_shape.z;
    if (mode < -2.5) { o_color = v_color; return; }
    if (mode < -1.5) { o_color = texture(u_tex, v_uv) * v_color; return; }
    if (mode < -0.5) { o_color = vec4(v_color.rgb, v_color.a * texture(u_tex, v_uv).r); return; }
    float d = sd_box(v_local, v_shape.xy, v_shape.z);
    float blur = v_uv.x; float a;
    if (blur > 0.0) { a = 1.0 - smoothstep(-blur, blur, d); }
    else {
        float aa = max(fwidth(d), 1e-4);
        a = clamp(0.5 - d / aa, 0.0, 1.0);
        if (v_shape.w > 0.0) a -= clamp(0.5 - (d + v_shape.w) / aa, 0.0, 1.0);
    }
    o_color = vec4(v_color.rgb, v_color.a * a);
}
)";
constexpr int kMaxTex = 64;
}  // namespace

struct Renderer {
    Shader shader{};
    int u_scale = -1;
    Texture2D atlas{};
    uint32_t atlas_version = 0;
    Texture2D tex[kMaxTex]{};
};

Renderer* create_renderer() {
    auto* r = new Renderer;
    r->shader = LoadShaderFromMemory(kVs, kFs);
    r->u_scale = GetShaderLocation(r->shader, "u_scale");
    return r;
}

void destroy_renderer(Renderer* r) {
    if (!r) return;
    UnloadShader(r->shader);
    if (r->atlas.id) UnloadTexture(r->atlas);
    delete r;
}

void set_texture(Renderer* r, TextureId id, Texture2D tex) {
    if (r && id > 0 && id < kMaxTex) r->tex[id] = tex;
}

static void put_utf8(Input& in, unsigned cp) {
    char b[4];
    int n = 0;
    if (cp < 0x80) {
        b[n++] = (char)cp;
    } else if (cp < 0x800) {
        b[n++] = (char)(0xC0 | (cp >> 6));
        b[n++] = (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        b[n++] = (char)(0xE0 | (cp >> 12));
        b[n++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        b[n++] = (char)(0x80 | (cp & 0x3F));
    } else {
        b[n++] = (char)(0xF0 | (cp >> 18));
        b[n++] = (char)(0x80 | ((cp >> 12) & 0x3F));
        b[n++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        b[n++] = (char)(0x80 | (cp & 0x3F));
    }
    for (int i = 0; i < n; i++) in.text.push(b[i]);
}

void fill_input(Input& in) {
    in = Input{};
    const Vector2 m = GetMousePosition();
    in.pointer = {m.x, m.y};
    in.pointer_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    in.pointer_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    in.pointer_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    in.secondary_pressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    in.wheel = GetMouseWheelMove();
    in.screen_size = {(float)GetScreenWidth(), (float)GetScreenHeight()};
    static const struct {
        int rk;
        Key k;
    } map[] = {{KEY_ENTER, KeyEnter},   {KEY_BACKSPACE, KeyBackspace},
               {KEY_DELETE, KeyDelete}, {KEY_LEFT, KeyLeft},
               {KEY_RIGHT, KeyRight},   {KEY_UP, KeyUp},
               {KEY_DOWN, KeyDown},     {KEY_HOME, KeyHome},
               {KEY_END, KeyEnd},       {KEY_TAB, KeyTab},
               {KEY_ESCAPE, KeyEscape}, {KEY_SPACE, KeySpace},
               {KEY_A, KeyA},           {KEY_C, KeyC},
               {KEY_V, KeyV},           {KEY_X, KeyX}};
    const bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    const bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    for (const auto& e : map)
        if (IsKeyPressed(e.rk) || IsKeyPressedRepeat(e.rk)) in.keys.push({e.k, shift, ctrl, false});
    for (int cp = GetCharPressed(); cp; cp = GetCharPressed()) put_utf8(in, (unsigned)cp);
}

void apply_dpi(Context& ctx) {
    const int sw = GetScreenWidth();
    ctx.set_dpi_scale(sw > 0 ? (float)GetRenderWidth() / (float)sw : 1.0f);
}

void render(Renderer* r, const Context& ctx) {
    const DrawList& dl = ctx.draw_list();
    const int ni = dl.index_count(), nc = dl.command_count();
    if (!r || ni == 0 || nc == 0) return;
    const float dpi = ctx.dpi_scale();
    const int fw = GetRenderWidth(), fh = GetRenderHeight();

    if (r->atlas_version != ctx.atlas_version() || r->atlas.id == 0) {
        const int n = ctx.atlas_size();
        if (r->atlas.id && r->atlas.width != n) {
            UnloadTexture(r->atlas);
            r->atlas = Texture2D{};
        }
        if (r->atlas.id == 0) {
            Image img{(void*)ctx.atlas_pixels(), n, n, 1, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE};
            r->atlas = LoadTextureFromImage(img);
            SetTextureFilter(r->atlas, TEXTURE_FILTER_BILINEAR);
        } else {
            UpdateTexture(r->atlas, ctx.atlas_pixels());
        }
        r->atlas_version = ctx.atlas_version();
    }

    // raylib's index buffers are 16-bit, so the list is expanded to a triangle soup (index order kept).
    // DrawVertex fields ride in the standard mesh attributes: local -> normals, shape -> tangents.
    Mesh m{};
    m.vertexCount = ni;
    m.triangleCount = ni / 3;
    m.vertices = (float*)MemAlloc((unsigned)ni * 3 * sizeof(float));
    m.texcoords = (float*)MemAlloc((unsigned)ni * 2 * sizeof(float));
    m.normals = (float*)MemAlloc((unsigned)ni * 3 * sizeof(float));
    m.colors = (unsigned char*)MemAlloc((unsigned)ni * 4);
    m.tangents = (float*)MemAlloc((unsigned)ni * 4 * sizeof(float));
    for (int i = 0; i < ni; i++) {
        const DrawVertex& v = dl.vertices()[dl.indices()[i]];
        m.vertices[i * 3] = v.pos.x;
        m.vertices[i * 3 + 1] = v.pos.y;
        m.vertices[i * 3 + 2] = 0;
        m.texcoords[i * 2] = v.uv.x;
        m.texcoords[i * 2 + 1] = v.uv.y;
        m.normals[i * 3] = v.local.x;
        m.normals[i * 3 + 1] = v.local.y;
        m.normals[i * 3 + 2] = 0;
        std::memcpy(&m.colors[i * 4], &v.color, 4);
        m.tangents[i * 4] = v.shape.x;
        m.tangents[i * 4 + 1] = v.shape.y;
        m.tangents[i * 4 + 2] = v.shape.z;
        m.tangents[i * 4 + 3] = v.shape.w;
    }
    rlDrawRenderBatchActive();  // flush raylib's own pending draws first
    UploadMesh(&m, false);

    rlDisableDepthTest();
    rlDisableBackfaceCulling();
    rlEnableScissorTest();
    rlEnableShader(r->shader.id);
    const float scale[2] = {2.0f * dpi / (float)fw, 2.0f * dpi / (float)fh};
    rlSetUniform(r->u_scale, scale, RL_SHADER_UNIFORM_VEC2, 1);
    rlEnableVertexArray(m.vaoId);
    unsigned bound = ~0u;
    for (int i = 0; i < nc; i++) {
        const DrawCommand& c = dl.commands()[i];
        float x0 = c.clip.x * dpi, y0 = c.clip.y * dpi;
        float x1 = (c.clip.x + c.clip.w) * dpi, y1 = (c.clip.y + c.clip.h) * dpi;
        x0 = x0 < 0 ? 0 : x0;
        y0 = y0 < 0 ? 0 : y0;
        x1 = x1 > (float)fw ? (float)fw : x1;
        y1 = y1 > (float)fh ? (float)fh : y1;
        if (x1 <= x0 || y1 <= y0) continue;
        rlScissor((int)x0, (int)((float)fh - y1), (int)(x1 - x0 + 0.5f), (int)(y1 - y0 + 0.5f));
        const unsigned want = (c.texture > 0 && c.texture < kMaxTex && r->tex[c.texture].id)
                                  ? r->tex[c.texture].id
                                  : r->atlas.id;
        if (want != bound) {
            rlActiveTextureSlot(0);
            rlEnableTexture(want);
            bound = want;
        }
        rlDrawVertexArray((int)c.index_offset, (int)c.index_count);
    }
    rlDisableVertexArray();
    rlDisableShader();
    rlDisableScissorTest();
    rlEnableDepthTest();
    UnloadMesh(m);
}

}  // namespace rd::rl
