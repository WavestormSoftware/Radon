// Copyright (c) 2026 Wavestorm Software
#include "runner.hpp"

#include <SDL3/SDL.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "radon_sdl3_gl.hpp"

namespace demo {

std::vector<uint8_t> read_file(const std::string& path) {
    std::vector<uint8_t> out;
    size_t n = 0;
    if (void* d = SDL_LoadFile(path.c_str(), &n)) {
        out.assign(static_cast<uint8_t*>(d), static_cast<uint8_t*>(d) + n);
        SDL_free(d);
    }
    return out;
}

std::vector<uint8_t> find_font(const char* explicit_path) {
    std::vector<std::string> tried;
    if (explicit_path) tried.push_back(explicit_path);
    if (const char* e = std::getenv("RD_FONT")) tried.push_back(e);
    for (const char* p : {"examples/assets/font.ttf", "assets/font.ttf", "font.ttf", "/font.ttf"})
        tried.push_back(p);
    // then a system UI font, so the demos run out of the box
    for (const char* p :
         {"/usr/share/fonts/noto/NotoSans-Regular.ttf", "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
          "/usr/share/fonts/TTF/DejaVuSans.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
          "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
          "/usr/share/fonts/liberation/LiberationSans-Regular.ttf", "C:/Windows/Fonts/segoeui.ttf",
          "C:/Windows/Fonts/arial.ttf", "/System/Library/Fonts/Supplemental/Arial.ttf",
          "/Library/Fonts/Arial.ttf", "/system/fonts/Roboto-Regular.ttf", "/system/fonts/DroidSans.ttf"})
        tried.push_back(p);
    if (const char* base = SDL_GetBasePath()) {
        tried.push_back(std::string(base) + "assets/font.ttf");
        tried.push_back(std::string(base) + "font.ttf");
    }
    for (const auto& p : tried) {
        auto f = read_file(p);
        if (!f.empty()) {
            std::printf("radon demo: font %s\n", p.c_str());
            return f;
        }
    }
    std::fprintf(
        stderr,
        "radon demo: ERROR: no font found. Put a TrueType font at examples/assets/font.ttf, set RD_FONT, "
        "or pass --font <path>. Running without text.\n");
    return {};
}

rd::Theme make_theme(const Fonts& f, bool dark) {
    rd::Theme t = dark ? rd::Theme::dark() : rd::Theme::light();
    t.font_body = f.body;
    t.font_heading = f.heading;
    t.font_small = f.small;
    t.font_large = f.large;
    return t;
}

struct Runner::Impl {
    Options opt;
    AppConfig cfg;
    std::vector<uint8_t> font;
    Fonts fonts;
    rd::sdl3::Platform* plat = nullptr;
    rd::gl::Renderer* rend = nullptr;
    std::unique_ptr<rd::Context> ctx;
    rd::Input input;
    int frame_no = 0;
    int exit_code = 0;
};

Runner::Runner() : m_(new Impl) {}
Runner::~Runner() {
    stop();
}
rd::Context& Runner::context() {
    return *m_->ctx;
}
const Options& Runner::options() const {
    return m_->opt;
}
const Fonts& Runner::fonts() const {
    return m_->fonts;
}
bool Runner::dark() const {
    if (m_->opt.light) return false;
    if (m_->opt.dark) return true;
    return m_->plat ? rd::sdl3::system_dark(m_->plat) : true;
}
void Runner::open_url(const char* url) {
    rd::sdl3::open_url(url);
}
void Runner::show_open_dialog() {
    rd::sdl3::show_file_dialog(m_->plat, rd::sdl3::FileDialog::Open);
}
bool Runner::poll_dialog(std::string& path) {
    return rd::sdl3::poll_file_dialog(m_->plat, path);
}
rd::sdl3::Platform* Runner::platform() {
    return m_->plat;
}
rd::gl::Renderer* Runner::renderer() {
    return m_->rend;
}
rd::TextureId Runner::create_texture(int w, int h, const uint8_t* rgba) {
    return rd::gl::create_texture(m_->rend, w, h, rgba);
}

bool Runner::start(int argc, char** argv, const AppConfig& cfg) {
    Impl& m = *m_;
    m.cfg = cfg;
    Options& o = m.opt;
    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        const bool more = i + 1 < argc;
        if (!std::strcmp(a, "--font") && more)
            o.font = argv[++i];
        else if (!std::strcmp(a, "--screenshot") && more)
            o.screenshot = argv[++i];
        else if (!std::strcmp(a, "--frames") && more)
            o.frames = std::atoi(argv[++i]);
        else if (!std::strcmp(a, "--page") && more)
            o.page = std::atoi(argv[++i]);
        else if (!std::strcmp(a, "--light"))
            o.light = true;
        else if (!std::strcmp(a, "--dark"))
            o.dark = true;
        else if (!std::strcmp(a, "--touch"))
            o.touch = true;
        else if (!std::strcmp(a, "--scale") && more)
            o.scale = (float)std::atof(argv[++i]);
        else if (!std::strcmp(a, "--size") && more)
            std::sscanf(argv[++i], "%dx%d", &o.width, &o.height);
        else if (!std::strcmp(a, "--pointer") && more)
            std::sscanf(argv[++i], "%f,%f", &o.pointer_x, &o.pointer_y);
        else if (!std::strcmp(a, "--click") && more)
            std::sscanf(argv[++i], "%f,%f", &o.click_x, &o.click_y);
        else if (!std::strcmp(a, "--help")) {
            std::printf(
                "options: --font <ttf> --screenshot <png> --frames <n> --page <n> --light --dark --touch "
                "--size WxH --scale k "
                "--pointer x,y --click x,y\n");
            return false;
        }
    }
    if (o.screenshot && o.frames <= 0) o.frames = 30;
    if (!(o.scale >= 1 && o.scale <= 4)) o.scale = 1;

    m.font = find_font(o.font);
    rd::sdl3::PlatformConfig pc;
    pc.title = cfg.title;
    pc.width = (int)((float)(o.width > 0 ? o.width : cfg.width) * o.scale);
    pc.height = (int)((float)(o.height > 0 ? o.height : cfg.height) * o.scale);
    pc.custom_titlebar = cfg.custom_titlebar;
    pc.transparent = cfg.transparent;
    pc.wait_events = !o.screenshot;  // sleep while idle; screenshots need a fixed number of frames
    m.plat = rd::sdl3::create_platform(pc);
    if (!m.plat) {
        std::fprintf(stderr, "radon demo: cannot create window/GL context: %s\n", SDL_GetError());
        return false;
    }
    rd::gl::RendererConfig rc;
    rc.gles = rd::sdl3::is_gles(m.plat);
    m.rend = rd::gl::create_renderer(rc);
    if (!m.rend) {
        std::fprintf(stderr, "radon demo: renderer setup failed\n");
        return false;
    }
    m.ctx.reset(new rd::Context(cfg.config));
    rd::sdl3::attach(m.plat, *m.ctx);
    if (!m.font.empty()) {
        m.fonts.body = m.ctx->load_font(m.font.data(), m.font.size(), 16);
        m.fonts.heading = m.ctx->load_font(m.font.data(), m.font.size(), 16);
        m.fonts.small = m.ctx->load_font(m.font.data(), m.font.size(), 13);
        m.fonts.large = m.ctx->load_font(m.font.data(), m.font.size(), 22);
        m.ctx->set_default_font(m.fonts.body);
    }
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IPHONE)
    o.touch = true;
#endif
    rd::Theme theme = make_theme(m.fonts, dark());
    m.ctx->set_theme(o.touch ? rd::Theme::touch(theme) : theme);
    if (cfg.init) cfg.init(*m.ctx, m.fonts, m.opt);
    return true;
}

bool Runner::step() {
    Impl& m = *m_;
    float dt = 0;
    if (!rd::sdl3::poll(m.plat, m.input, dt)) return false;
    if (rd::sdl3::skip_frame(m.plat)) return true;  // web: nothing changed, keep the last frame
    if (rd::sdl3::system_theme_changed(m.plat) && !m.opt.light && !m.opt.dark) {
        if (m.cfg.system_theme)
            m.cfg.system_theme(*m.ctx, dark());
        else
            m.ctx->set_theme(make_theme(m.fonts, dark()));
    }
    int fw, fh;
    rd::sdl3::framebuffer_size(m.plat, fw, fh);
    const float dpi = rd::sdl3::dpi_scale(m.plat) * m.opt.scale;
    m.ctx->set_dpi_scale(dpi);
    const int lw = (int)((float)fw / dpi), lh = (int)((float)fh / dpi);
    m.input.screen_size = {(float)lw, (float)lh};
    if (m.opt.screenshot) {  // scripted pointer and a fixed time step for reproducible screenshots
        dt = 1.0f / 60;
        m.input.pointer = {m.opt.pointer_x, m.opt.pointer_y};
        m.input.pointer_down = m.input.pointer_pressed = m.input.pointer_released = false;
        if (m.opt.click_x >= 0 && m.frame_no >= 3 && m.frame_no <= 6) {
            m.input.pointer = {m.opt.click_x, m.opt.click_y};
            m.input.pointer_down = m.input.pointer_pressed = m.frame_no == 5;
            m.input.pointer_released = m.frame_no == 6;
        }
    }
    m.ctx->begin_frame(m.input, dt);
    m.cfg.frame(*m.ctx, m.input, lw, lh);
    m.ctx->end_frame();
    rd::gl::clear(m.rend, m.cfg.transparent ? rd::Color{0, 0, 0, 0} : m.ctx->theme().background);
    rd::gl::render(m.rend, *m.ctx, fw, fh);
    if (m.opt.screenshot && ++m.frame_no >= m.opt.frames) {
        std::vector<uint8_t> px((size_t)fw * fh * 4);
        rd::gl::read_pixels(m.rend, fw, fh, px.data());
        SDL_Surface* s = SDL_CreateSurfaceFrom(fw, fh, SDL_PIXELFORMAT_RGBA32, px.data(), fw * 4);
#if SDL_VERSION_ATLEAST(3, 4, 0)
        const bool saved = s && SDL_SavePNG(s, m.opt.screenshot);
#else
        const bool saved = s && SDL_SaveBMP(s, m.opt.screenshot);  // SDL < 3.4 has no PNG writer
#endif
        if (saved) {
            std::printf("radon demo: wrote %s (%dx%d)\n", m.opt.screenshot, fw, fh);
        } else {
            std::fprintf(stderr, "radon demo: screenshot failed: %s\n", SDL_GetError());
            m.exit_code = 1;
        }
        if (s) SDL_DestroySurface(s);
        return false;
    }
    rd::sdl3::finish_frame(m.plat, *m.ctx);
    return true;
}

void Runner::stop() {
    Impl& m = *m_;
    m.ctx.reset();
    if (m.rend) rd::gl::destroy_renderer(m.rend);
    if (m.plat) rd::sdl3::destroy_platform(m.plat);
    m.rend = nullptr;
    m.plat = nullptr;
}

std::vector<uint8_t> checker_rgba(int n, rd::Color a, rd::Color b) {
    std::vector<uint8_t> px((size_t)n * n * 4);
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            const rd::Color c = (((x * 4 / n) + (y * 4 / n)) & 1) ? a : b;
            uint8_t* p = &px[((size_t)y * n + x) * 4];
            p[0] = c.r, p[1] = c.g, p[2] = c.b, p[3] = 255;
        }
    return px;
}

std::vector<uint8_t> art_rgba(int w, int h) {  // a pink-to-navy sunset with sun and stripes
    std::vector<uint8_t> px((size_t)w * h * 4);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const float fy = (float)y / (float)h, fx = (float)x / (float)w;
            float r = 20 + 220 * (1 - fy) * (0.6f + 0.4f * fx), g = 14 + 40 * (1 - fy),
                  b = 50 + 110 * (1 - fy);
            const float dx = fx - 0.7f, dy = (fy - 0.45f) * (float)h / (float)w;
            if (fy > 0.7f && ((y * 32 / h) & 1)) r *= 0.55f, g *= 0.55f, b *= 0.7f;
            // sun with an anti-aliased edge (one pixel wide) and a soft glow around it
            const float d = std::sqrt(dx * dx + dy * dy) * (float)w, rad = 0.11f * (float)w;
            const float sun = std::fmin(1.0f, std::fmax(0.0f, rad - d + 0.5f)) * (fy < 0.7f ? 1.0f : 0.35f);
            const float glow = 0.35f * std::exp(-std::fmax(0.0f, d - rad) / (0.08f * (float)w));
            r += (255 - r) * glow, g += (90 - g) * glow * 0.6f, b += (170 - b) * glow;
            r += (255 - r) * sun, g += (130 - g) * sun, b += (180 - b) * sun;
            uint8_t* p = &px[((size_t)y * w + x) * 4];
            p[0] = (uint8_t)(r > 255 ? 255 : r), p[1] = (uint8_t)g, p[2] = (uint8_t)(b > 255 ? 255 : b),
            p[3] = 255;
        }
    return px;
}

int run(int argc, char** argv, const AppConfig& cfg) {
    Runner r;
    if (!r.start(argc, argv, cfg)) return 1;
    while (r.step()) {
    }
    int code = r.m_->exit_code;
    r.stop();
    return code;
}

}  // namespace demo
