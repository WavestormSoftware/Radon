// Copyright (c) 2026 Wavestorm Software
#pragma once
#include <memory>
#include <radon/types.hpp>

namespace rd {

/// SDF shape data: `shape` = (half width, half height, radius, border). `uv.x` is the shadow blur
/// for rect shapes. `shape.z` < 0 selects a non-SDF mode: -1 glyph coverage, -2 RGBA image, -3 solid.
struct DrawVertex {
    Vec2 pos;
    Vec2 uv;
    Color color;
    Vec2 local;  ///< position relative to the shape center
    Vec4 shape;
    Color color2{};  ///< gradient end color
    Vec4 radii;      ///< per-corner radii (tl, tr, br, bl) for rect shapes
    Vec4 grad;       ///< gradient: z = 0 none, 1 linear (xy = direction / extent), 2 radial (xy = center, w = 1 / radius)
};

/// ShapeLine (-4): anti-aliased capsule; `local` = (along, across) the segment, shape.xy = (half length, half width).
enum : int { ShapeGlyph = -1, ShapeImage = -2, ShapeSolid = -3, ShapeLine = -4 };

struct DrawCommand {
    TextureId texture = 0;
    Rect clip;
    uint32_t index_offset = 0;
    uint32_t index_count = 0;
    uint32_t layer = 0;  ///< internal: commands are stable-sorted by layer (window z-order)
};

class DrawList {
public:
    DrawList(int max_commands, int max_vertices, int max_indices);

    void clear();
    const DrawVertex* vertices() const { return verts_.get(); }
    int vertex_count() const { return nv_; }
    const uint32_t* indices() const { return idx_.get(); }
    int index_count() const { return ni_; }
    const DrawCommand* commands() const { return cmds_.get(); }
    int command_count() const { return nc_; }
    bool overflow() const { return overflow_; }

    void set_clip(const Rect& r) { clip_ = r; }
    void set_layer(uint32_t layer) { layer_ = layer; }
    void break_command() { force_new_ = true; }
    /// Multiplies the alpha of everything drawn afterwards (fades, disabled sections). 1 = opaque.
    void set_alpha(float a) { alpha_ = a < 0 ? 0 : (a > 1 ? 1 : a); }
    float alpha() const { return alpha_; }
    void sort_by_layer();

    /// Rounded rectangle. border > 0 draws only a ring of that width.
    void rect(const Rect& r, Color c, float radius = 0, float border = 0);
    void shadow(const Rect& r, Color c, float radius, float blur, Vec2 offset);
    /// Rounded rectangle with a Paint fill (solid or gradient), per-corner radii and an optional ring.
    void fill(const Rect& r, const Paint& p, const Corners& radius = {}, float border = 0);
    /// A complete modern box: shadow, fill, border, corner radii and opacity from one Style.
    void box(const Rect& r, const Style& st);
    void glyph(const Rect& quad, const Rect& uv, Color c);
    void image(TextureId tex, const Rect& quad, const Rect& uv, Color tint);
    /// Axis-aligned quad with a color per corner (top-left, top-right, bottom-right, bottom-left).
    void gradient_rect(const Rect& r, Color tl, Color tr, Color br, Color bl);
    void line(Vec2 a, Vec2 b, Color c, float thickness = 1);
    void triangle(Vec2 a, Vec2 b, Vec2 c, Color col);

private:
    bool begin(TextureId tex, int nverts, int nidx);
    void quad(TextureId tex, const Rect& pos, Vec2 uv0, Vec2 uv1, Color c, Vec2 center, Vec4 shape,
              Color c2 = {}, Vec4 radii = {}, Vec4 grad = {});
    void rect_shape(const Rect& r, const Paint& p, const Corners& radius, float border, float margin, float blur,
                    Vec2 center_offset);

    std::unique_ptr<DrawVertex[]> verts_;
    std::unique_ptr<uint32_t[]> idx_;
    std::unique_ptr<DrawCommand[]> cmds_;
    int max_c_, max_v_, max_i_;
    int nv_ = 0, ni_ = 0, nc_ = 0;
    bool overflow_ = false, force_new_ = true;
    Rect clip_{-1e6f, -1e6f, 2e6f, 2e6f};
    uint32_t layer_ = 0;
    float alpha_ = 1;
    Color fade(Color c) const {
        c.a = (uint8_t)((float)c.a * alpha_ + 0.5f);
        return c;
    }
};

}  // namespace rd
