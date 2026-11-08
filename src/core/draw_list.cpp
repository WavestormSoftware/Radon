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
                last->clip.h == clip_.h;
    if (!same) {
        if (nc_ >= max_c_) {
            overflow_ = true;
            return false;
        }
        cmds_[nc_++] = DrawCommand{tex, clip_, (uint32_t)ni_, 0, layer_};
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

void DrawList::quad(TextureId tex, const Rect& r, Vec2 uv0, Vec2 uv1, Color c, Vec2 center,
                    Vec4 shape) {
    if (!begin(tex, 4, 6)) return;
    const float xs[4] = {r.x, r.x + r.w, r.x + r.w, r.x};
    const float ys[4] = {r.y, r.y, r.y + r.h, r.y + r.h};
    const float us[4] = {uv0.x, uv1.x, uv1.x, uv0.x};
    const float vs[4] = {uv0.y, uv0.y, uv1.y, uv1.y};
    uint32_t base = nv_;
    for (int i = 0; i < 4; i++)
        verts_[nv_++] = DrawVertex{{xs[i], ys[i]}, {us[i], vs[i]}, c, {xs[i] - center.x, ys[i] - center.y}, shape};
    static const uint32_t order[6] = {0, 1, 2, 0, 2, 3};
    for (uint32_t o : order) idx_[ni_++] = base + o;
}

void DrawList::rect_shape(const Rect& r, Color c, float radius, float border, float margin,
                          float blur, Vec2 off) {
    if (c.a == 0 || r.w <= 0 || r.h <= 0) return;
    Rect q{r.x - margin + off.x, r.y - margin + off.y, r.w + 2 * margin, r.h + 2 * margin};
    Vec2 center{r.x + r.w / 2 + off.x, r.y + r.h / 2 + off.y};
    float maxr = std::fmin(r.w, r.h) / 2;
    quad(0, q, {blur, 0}, {blur, 0}, c, center, {r.w / 2, r.h / 2, std::fmin(radius, maxr), border});
}

void DrawList::rect(const Rect& r, Color c, float radius, float border) {
    rect_shape(r, c, radius, border, 1.0f, 0, {0, 0});
}

void DrawList::shadow(const Rect& r, Color c, float radius, float blur, Vec2 offset) {
    rect_shape(r, c, radius, 0, blur + 1.0f, blur, offset);
}

void DrawList::glyph(const Rect& q, const Rect& uv, Color c) {
    quad(0, q, {uv.x, uv.y}, {uv.w, uv.h}, c, {0, 0}, {0, 0, (float)ShapeGlyph, 0});
}

void DrawList::image(TextureId tex, const Rect& q, const Rect& uv, Color tint) {
    quad(tex, q, {uv.x, uv.y}, {uv.x + uv.w, uv.y + uv.h}, tint, {0, 0}, {0, 0, (float)ShapeImage, 0});
}

void DrawList::triangle(Vec2 a, Vec2 b, Vec2 c, Color col) {
    if (!begin(0, 3, 3)) return;
    Vec4 sh{0, 0, (float)ShapeSolid, 0};
    uint32_t base = nv_;
    for (Vec2 p : {a, b, c}) verts_[nv_++] = DrawVertex{p, {0, 0}, col, {0, 0}, sh};
    for (uint32_t i = 0; i < 3; i++) idx_[ni_++] = base + i;
}

void DrawList::line(Vec2 a, Vec2 b, Color c, float t) {
    float dx = b.x - a.x, dy = b.y - a.y, len = std::sqrt(dx * dx + dy * dy);
    if (len <= 0) return;
    float nx = -dy / len * t / 2, ny = dx / len * t / 2;
    triangle({a.x + nx, a.y + ny}, {b.x + nx, b.y + ny}, {b.x - nx, b.y - ny}, c);
    triangle({a.x + nx, a.y + ny}, {b.x - nx, b.y - ny}, {a.x - nx, a.y - ny}, c);
}

}  // namespace rd
