// Copyright (c) 2026 Wavestorm Software
#pragma once
#include <radon/types.hpp>

namespace rd {

class Context;

/// Decodes one UTF-8 code point from [p, end). Returns the byte length (>= 1). Invalid or truncated
/// input yields U+FFFD with length 1, so callers always make progress.
int utf8_decode(const char* p, const char* end, uint32_t& cp);

/// Width and line height of `text` (a single line) in logical pixels. Returns {0, cell height} when
/// no font is available.
Vec2 measure_text(const Context& ctx, FontHandle font, std::string_view text);

struct TextLine {
    int start = 0;  ///< byte offset into the source text
    int length = 0; ///< bytes, without the trailing space or newline
};

/// Splits `text` into lines no wider than `max_width`. Breaks at spaces, then inside long words at
/// code point boundaries; '\n' always breaks. Writes at most `max_lines` lines and returns the count
/// that would be needed, clamped to `max_lines`.
int wrap_text(const Context& ctx, FontHandle font, std::string_view text, float max_width,
              TextLine* lines, int max_lines);

namespace detail {
/// Wrapping with a caller-supplied per-code-point advance (used by tests and wrap_text).
using AdvanceFn = float (*)(void* user, uint32_t cp);
int wrap_with(std::string_view text, float max_width, AdvanceFn adv, void* user, TextLine* lines,
              int max_lines);
}  // namespace detail

}  // namespace rd
