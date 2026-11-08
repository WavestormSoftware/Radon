# 0005: What the allocation test counts

`test_alloc` replaces global `operator new` and, on glibc without ASan, interposes `malloc`, `calloc` and
`realloc`. On other platforms it counts `operator new` only. stb_truetype's internal `malloc` calls are
redirected to a preallocated arena (`STBTT_malloc`), so glyph rasterization does not touch the heap even
during warm-up. The test warms up every glyph, window, text edit and animation slot first, then counts 200
frames with moving pointer, wheel, key and text input.
