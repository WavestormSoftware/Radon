// Copyright (c) 2026 Wavestorm Software
#include "shaders.hpp"

namespace rd::gl {

// The renderer prepends "#version 330 core" (desktop) or "#version 300 es" + precision (WebGL2/GLES3).
const char* const kVertexBody = R"(
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;
layout(location = 3) in vec2 a_local;
layout(location = 4) in vec4 a_shape;
uniform vec2 u_scale;
out vec2 v_uv;
out vec4 v_color;
out vec2 v_local;
out vec4 v_shape;
void main() {
    v_uv = a_uv;
    v_color = a_color;
    v_local = a_local;
    v_shape = a_shape;
    gl_Position = vec4(a_pos.x * u_scale.x - 1.0, 1.0 - a_pos.y * u_scale.y, 0.0, 1.0);
}
)";

// shape = (half w, half h, radius, border). shape.z < 0 selects glyph (-1), image (-2) or solid (-3).
// For rects, uv.x carries the shadow blur.
const char* const kFragmentBody = R"(
uniform sampler2D u_tex;
in vec2 v_uv;
in vec4 v_color;
in vec2 v_local;
in vec4 v_shape;
out vec4 o_color;

float sd_box(vec2 p, vec2 half_size, float r) {
    vec2 q = abs(p) - half_size + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    float mode = v_shape.z;
    if (mode < -3.5) {  // capsule line: local = (along, across), shape.xy = (half length, half thickness)
        vec2 q = vec2(max(abs(v_local.x) - v_shape.x, 0.0), v_local.y);
        float d = length(q) - v_shape.y;
        float aa = max(fwidth(d), 1e-4);
        o_color = vec4(v_color.rgb, v_color.a * clamp(0.5 - d / aa, 0.0, 1.0));
        return;
    }
    if (mode < -2.5) { o_color = v_color; return; }
    if (mode < -1.5) { o_color = texture(u_tex, v_uv) * v_color; return; }
    if (mode < -0.5) { o_color = vec4(v_color.rgb, v_color.a * texture(u_tex, v_uv).r); return; }
    float d = sd_box(v_local, v_shape.xy, v_shape.z);
    float blur = v_uv.x;
    float a;
    if (blur > 0.0) {
        a = 1.0 - smoothstep(-blur, blur, d);
    } else {
        float aa = max(fwidth(d), 1e-4);
        a = clamp(0.5 - d / aa, 0.0, 1.0);
        if (v_shape.w > 0.0) a -= clamp(0.5 - (d + v_shape.w) / aa, 0.0, 1.0);
    }
    o_color = vec4(v_color.rgb, v_color.a * a);
}
)";

}  // namespace rd::gl
