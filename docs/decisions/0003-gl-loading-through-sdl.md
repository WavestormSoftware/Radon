# 0003: GL entry points through SDL, own constants

The renderer declares the ~45 GL functions and constants it needs and loads them with
`SDL_GL_GetProcAddress`. No GL headers, loader library or extra dependency is needed, and the same code
runs on desktop GL 3.3 core, GLES3, and WebGL2 (which needs `-sGL_ENABLE_GET_PROC_ADDRESS=1`).
Desktop asks for a 3.3 core context and falls back to GLES 3.0 if that fails; mobile and web ask for GLES.
The "OpenGL ES 3 subset" shader is compiled with a `#version` header chosen at runtime.
