cmake_minimum_required(VERSION 3.20)
project(Radon VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR ANDROID OR IOS)
    set(_rd_desktop OFF)
else()
    set(_rd_desktop ON)
endif()
option(RD_BUILD_EXAMPLES "Build the demos" ${_rd_desktop})
option(RD_BUILD_TESTS "Build the tests" ${_rd_desktop})
option(RD_BACKEND_SDL3_GL "Build the SDL3 + OpenGL ES 3 backend" ON)
option(RD_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

# ---- core (no platform dependencies) ----
add_library(radon
    src/core/context.cpp src/core/ids.cpp src/core/layout.cpp src/core/window.cpp
    src/core/draw_list.cpp src/core/text.cpp src/core/theme.cpp
    src/core/widgets/basic.cpp src/core/widgets/value.cpp src/core/widgets/popup.cpp
    src/core/widgets/textbox.cpp src/core/widgets/extra.cpp src/core/widgets/icons.cpp
    src/core/widgets/container.cpp src/core/widgets/color.cpp)
add_library(radon::radon ALIAS radon)
target_include_directories(radon PUBLIC include)
target_include_directories(radon SYSTEM PRIVATE src/third_party)
target_compile_features(radon PUBLIC cxx_std_17)

function(rd_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(RD_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if(RD_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
rd_warnings(radon)

# ---- backend ----
if(RD_BACKEND_SDL3_GL)
    include(FetchContent)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.8
        GIT_SHALLOW TRUE
        FIND_PACKAGE_ARGS 3.2 CONFIG)  # prefer an installed SDL3
    FetchContent_MakeAvailable(SDL3)
    add_library(radon_sdl3_gl
        backends/sdl3_gl/platform_sdl3.cpp backends/sdl3_gl/renderer_gl.cpp
        backends/sdl3_gl/shaders.cpp)
    add_library(radon::sdl3_gl ALIAS radon_sdl3_gl)
    target_include_directories(radon_sdl3_gl PUBLIC backends/sdl3_gl)
    target_link_libraries(radon_sdl3_gl PUBLIC radon SDL3::SDL3)
    if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR ANDROID OR IOS OR WIN32 OR APPLE))
        find_package(OpenGL QUIET)
    endif()
    rd_warnings(radon_sdl3_gl)
endif()

if(RD_BUILD_EXAMPLES AND RD_BACKEND_SDL3_GL)
    add_subdirectory(examples)
endif()

if(RD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
