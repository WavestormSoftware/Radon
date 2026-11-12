// Copyright (c) 2026 Wavestorm Software
#include <radon/draw_list.hpp>
#include <cmath>

namespace rd {

DrawList::DrawList(int max_commands, int max_vertices, int max_indices)
    : verts_(new DrawVertex[max_vertices]),
      idx_(new uint32_t[max_indices]),
      cmds_(new DrawCommand[max_commands]),
      max_c_(max_commands),
      max_v_(max_vertices),
      max_i_(max_indices) {}

void DrawList::clear() {
    nv_ = ni_ = nc_ = 0;
    overflow_ = false;
    force_new_ = true;
    layer_ = 0;
    alpha_ = 1;
    clip_ = Rect{-1e6f, -1e6f, 2e6f, 2e6f};
}

bool DrawList::begin(TextureId tex, int nverts, int nidx) {
    if (nv_ + nverts > max_v_ || ni_ + nidx > max_i_) {
        overflow_ = true;
        return false;
    }
    DrawCommand* last = nc_ ? &cmds_[nc_ - 1] : nullptr;
    bool same = last && !force_new_ && last->texture == tex && last->layer == layer_ &&
                last->clip.x == clip_.x && last->clip.y == clip_.y && last->clip.w == clip_.w &&
                last->clip.h == clip_.h && last->blur == blur_;
    if (!same) {
        if (nc_ >= max_c_) {
            overflow_ = true;
            return false;
        }
        cmds_[nc_++] = DrawCommand{tex, clip_, (uint32_t)ni_, 0, layer_, blur_};
        force_new_ = false;
    }
    cmds_[nc_ - 1].index_count += nidx;
    return true;
}

void DrawList::sort_by_layer() {
    for (int i = 1; i < nc_; i++) {
        DrawCommand c = cmds_[i];
        int j = i - 1;
        while (j >= 0 && cmds_[j].layer > c.layer) {
            cmds_[j + 1] = cmds_[j];
            j--;
        }
        cmds_[j + 1] = c;
    }
}

void DrawList::quad(TextureId tex, const Rect& r, Vec2 uv0, Vec2 uv1, Color c, Vec2 center, Vec4 shape, Color c2,
                    Vec4 radii, Vec4 grad) {
    if (!begin(tex, 4, 6)) return;
    c = fade(c);
    const float xs[4] = {r.x, r.x + r.w, r.x + r.w, r.x};
    const float ys[4] = {r.y, r.y, r.y + r.h, r.y + r.h};
    const float us[4] = {uv0.x, uv1.x, uv1.x, uv0.x};
    const float vs[4] = {uv0.y, uv0.y, uv1.y, uv1.y};
    uint32_t base = nv_;
    for (int i = 0; i < 4; i++)
        verts_[nv_++] =
            DrawVertex{{xs[i], ys[i]}, {us[i], vs[i]}, c, {xs[i] - center.x, ys[i] - center.y}, shape, fade(c2), radii, grad};
    static const uint32_t order[6] = {0, 1, 2, 0, 2, 3};
    for (uint32_t o : order) idx_[ni_++] = base + o;
}

void DrawList::rect_shape(const Rect& r, const Paint& p, const Corners& radius, float border, float margin,
                          float blur, Vec2 off) {
    if (r.w <= 0 || r.h <= 0 || (p.a.a == 0 && (p.kind == Paint::Solid || p.b.a == 0))) return;
    Rect q{r.x - margin + off.x, r.y - margin + off.y, r.w + 2 * margin, r.h + 2 * margin};
    Vec2 center{r.x + r.w / 2 + off.x, r.y + r.h / 2 + off.y};
    const float maxr = std::fmin(r.w, r.h) / 2;
    const Vec4 rad{std::fmin(radius.tl, maxr), std::fmin(radius.tr, maxr), std::fmin(radius.br, maxr),
                   std::fmin(radius.bl, maxr)};
    Vec4 grad{};
    if (p.kind == Paint::Linear) {
        const float rd = p.angle * 3.14159265f / 180.0f, dx = std::cos(rd), dy = std::sin(rd);
        const float ext = std::fmax(std::fabs(dx) * r.w + std::fabs(dy) * r.h, 1e-3f);
        grad = {dx / ext, dy / ext, 1, 0};
    } else if (p.kind == Paint::Radial) {
        const float rr = std::fmax(p.radius * std::fmax(r.w, r.h), 1e-3f);
        grad = {(p.center.x - 0.5f) * r.w, (p.center.y - 0.5f) * r.h, 2, 1.0f / rr};
    }
    quad(0, q, {blur, 0}, {blur, 0}, p.a, center, {r.w / 2, r.h / 2, std::fmax(rad.x, std::fmax(rad.y, std::fmax(rad.z, rad.w))), border}, p.b, rad, grad);
}

void DrawList::rect(const Rect& r, Color c, float radius, float border) {
    rect_shape(r, c, radius, border, 1.0f, 0, {0, 0});
}

void DrawList::fill(const Rect& r, const Paint& p, const Corners& radius, float border) {
    rect_shape(r, p, radius, border, 1.0f, 0, {0, 0});
}

void DrawList::shadow(const Rect& r, Color c, float radius, float blur, Vec2 offset) {
    rect_shape(r, c, radius, 0, blur + 1.0f, blur, offset);
}

void DrawList::blur(const Rect& r, float radius, const Corners& corners) {
    if (radius <= 0 || r.w <= 0 || r.h <= 0) return;
    const float maxr = std::fmin(r.w, r.h) / 2;
    const Vec4 rad{std::fmin(corners.tl, maxr), std::fmin(corners.tr, maxr), std::fmin(corners.br, maxr),
                   std::fmin(corners.bl, maxr)};
    blur_ = std::fmin(radius, 32.0f);
    quad(0, {r.x - 1, r.y - 1, r.w + 2, r.h + 2}, {0, 0}, {0, 0}, {255, 255, 255, 255},
         {r.x + r.w / 2, r.y + r.h / 2}, {r.w / 2, r.h / 2, (float)ShapeBlur, 0}, {}, rad, {});
    blur_ = 0;
}

void DrawList::box(const Rect& r, const Style& st) {
    const float saved = alpha_;
    alpha_ *= st.opacity;
    if (st.shadow.color.a > 0) {
        const float g = st.shadow.spread;
        Rect sr{r.x - g, r.y - g, r.w + 2 * g, r.h + 2 * g};
        rect_shape(sr, st.shadow.color, st.radius.max() + g, 0, st.shadow.blur + 1.0f, st.shadow.blur, st.shadow.offset);
    }
    if (st.backdrop_blur > 0) blur(r, st.backdrop_blur, st.radius);
    fill(r, st.fill, st.radius);
    if (st.border > 0) fill(r, st.border_paint, st.radius, st.border);
    alpha_ = saved;
}

void DrawList::glyph(const Rect& q, const Rect& uv, Color c) {
    quad(0, q, {uv.x, uv.y}, {uv.w, uv.h}, c, {0, 0}, {0, 0, (float)ShapeGlyph, 0});
}

void DrawList::image(TextureId tex, const Rect& q, const Rect& uv, Color tint) {
    quad(tex, q, {uv.x, uv.y}, {uv.x + uv.w, uv.y + uv.h}, tint, {0, 0}, {0, 0, (float)ShapeImage, 0});
}

void DrawList::gradient_rect(const Rect& r, Color tl, Color tr, Color br, Color bl) {
    if (r.w <= 0 || r.h <= 0 || !begin(0, 4, 6)) return;
    const Vec4 sh{0, 0, (float)ShapeSolid, 0};
    const Vec2 p[4] = {{r.x, r.y}, {r.x + r.w, r.y}, {r.x + r.w, r.y + r.h}, {r.x, r.y + r.h}};
    const Color c[4] = {tl, tr, br, bl};
    uint32_t base = nv_;
    for (int i = 0; i < 4; i++) verts_[nv_++] = DrawVertex{p[i], {0, 0}, fade(c[i]), {0, 0}, sh, {}, {}, {}};
    static const uint32_t order[6] = {0, 1, 2, 0, 2, 3};
    for (uint32_t o : order) idx_[ni_++] = base + o;
}

void DrawList::triangle(Vec2 a, Vec2 b, Vec2 c, Color col) {
    if (!begin(0, 3, 3)) return;
    col = fade(col);
    Vec4 sh{0, 0, (float)ShapeSolid, 0};
    uint32_t base = nv_;
    for (Vec2 p : {a, b, c}) verts_[nv_++] = DrawVertex{p, {0, 0}, col, {0, 0}, sh, {}, {}, {}};
    for (uint32_t i = 0; i < 3; i++) idx_[ni_++] = base + i;
}

void DrawList::line(Vec2 a, Vec2 b, Color c, float t) {
    float dx = b.x - a.x, dy = b.y - a.y, len = std::sqrt(dx * dx + dy * dy);
    if (len <= 0) return;
    if (c.a == 0 || !begin(0, 4, 6)) return;
    c = fade(c);
    // Oriented quad with a 1px margin; the shader carves a round-capped capsule out of it.
    const float ux = dx / len, uy = dy / len, hl = len / 2, ht = t / 2, m = 1.0f;
    const Vec2 mid{(a.x + b.x) / 2, (a.y + b.y) / 2};
    const float sx[4] = {-1, 1, 1, -1}, sy[4] = {-1, -1, 1, 1};
    const Vec4 sh{hl, ht, (float)ShapeLine, 0};
    uint32_t base = nv_;
    for (int i = 0; i < 4; i++) {
        const float lx = sx[i] * (hl + ht + m), ly = sy[i] * (ht + m);
        verts_[nv_++] = DrawVertex{{mid.x + ux * lx - uy * ly, mid.y + uy * lx + ux * ly}, {0, 0}, c, {lx, ly}, sh, {}, {}, {}};
    }
    static const uint32_t order[6] = {0, 1, 2, 0, 2, 3};
    for (uint32_t o : order) idx_[ni_++] = base + o;
}

}  // namespace rd
