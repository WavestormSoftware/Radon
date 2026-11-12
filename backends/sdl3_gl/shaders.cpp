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
layout(location = 5) in vec4 a_color2;
layout(location = 6) in vec4 a_radii;
layout(location = 7) in vec4 a_grad;
uniform vec2 u_scale;
out vec2 v_uv;
out vec4 v_color;
out vec2 v_local;
out vec4 v_shape;
out vec4 v_color2;
out vec4 v_radii;
out vec4 v_grad;
void main() {
    v_uv = a_uv;
    v_color = a_color;
    v_local = a_local;
    v_shape = a_shape;
    v_color2 = a_color2;
    v_radii = a_radii;
    v_grad = a_grad;
    gl_Position = vec4(a_pos.x * u_scale.x - 1.0, 1.0 - a_pos.y * u_scale.y, 0.0, 1.0);
}
)";

// shape = (half w, half h, radius, border). shape.z < 0 selects glyph (-1), image (-2) or solid (-3).
// For rects, uv.x carries the shadow blur.
const char* const kFragmentBody = R"(
uniform sampler2D u_tex;
uniform sampler2D u_blur;  // blurred framebuffer, texture unit 1
uniform vec2 u_fb;         // framebuffer size in pixels
in vec2 v_uv;
in vec4 v_color;
in vec2 v_local;
in vec4 v_shape;
in vec4 v_color2;
in vec4 v_radii;
in vec4 v_grad;
out vec4 o_color;

// r = (top-left, top-right, bottom-right, bottom-left)
float sd_box(vec2 p, vec2 half_size, vec4 radii) {
    float r = p.x < 0.0 ? (p.y < 0.0 ? radii.x : radii.w) : (p.y < 0.0 ? radii.y : radii.z);
    vec2 q = abs(p) - half_size + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    float mode = v_shape.z;
    if (mode < -4.5) {  // frosted backdrop: the blurred frame, masked by the rounded rect
        float d = sd_box(v_local, v_shape.xy, v_radii);
        float aa = max(fwidth(d), 1e-4);
        o_color = vec4(texture(u_blur, gl_FragCoord.xy / u_fb).rgb, v_color.a * clamp(0.5 - d / aa, 0.0, 1.0));
        return;
    }
    if (mode < -3.5) {  // capsule line: local = (along, across), shape.xy = (half length, half thickness)
        vec2 q = vec2(max(abs(v_local.x) - v_shape.x, 0.0), v_local.y);
        float d = length(q) - v_shape.y;
        float aa = max(fwidth(d), 1e-4);
        o_color = vec4(v_color.rgb, v_color.a * clamp(0.5 - d / aa, 0.0, 1.0));
        return;
    }
    if (mode < -2.5) { o_color = v_color; return; }
    if (mode < -1.5) { o_color = texture(u_tex, v_uv) * v_color; return; }
    if (mode < -0.5) {  // SDF glyph: 0.5 is the outline; fwidth gives a one-pixel antialiasing band
        float sd = texture(u_tex, v_uv).r;
        float w = max(fwidth(sd), 1e-4) * 0.75;
        o_color = vec4(v_color.rgb, v_color.a * smoothstep(0.5 - w, 0.5 + w, sd));
        return;
    }
    float d = sd_box(v_local, v_shape.xy, v_radii);
    float blur = v_uv.x;
    float a;
    if (blur > 0.0) {
        a = 1.0 - smoothstep(-blur, blur, d);
    } else {
        float aa = max(fwidth(d), 1e-4);
        a = clamp(0.5 - d / aa, 0.0, 1.0);
        if (v_shape.w > 0.0) a -= clamp(0.5 - (d + v_shape.w) / aa, 0.0, 1.0);
    }
    vec4 col = v_color;
    if (v_grad.z > 0.5) {  // 1 = linear, 2 = radial; dithered so slow gradients do not band
        float t = v_grad.z < 1.5 ? dot(v_local, v_grad.xy) + 0.5 : length(v_local - v_grad.xy) * v_grad.w;
        col = mix(v_color, v_color2, clamp(t, 0.0, 1.0));
        col.rgb += (fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715)))) - 0.5) / 255.0;
    }
    o_color = vec4(col.rgb, col.a * a);
}
)";

}  // namespace rd::gl

namespace rd::gl {

// Separable 9-tap Gaussian (5 fetches via linear sampling). `u_step` is the sample spacing in uv units.
const char* const kBlurVertexBody = R"(
out vec2 v_uv;
void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    v_uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";

const char* const kBlurFragmentBody = R"(
uniform sampler2D u_tex;
uniform vec2 u_step;
in vec2 v_uv;
out vec4 o_color;
void main() {
    vec4 c = texture(u_tex, v_uv) * 0.2270270270;
    c += (texture(u_tex, v_uv + u_step * 1.3846153846) + texture(u_tex, v_uv - u_step * 1.3846153846)) * 0.3162162162;
    c += (texture(u_tex, v_uv + u_step * 3.2307692308) + texture(u_tex, v_uv - u_step * 3.2307692308)) * 0.0702702703;
    o_color = c;
}
)";

}  // namespace rd::gl
