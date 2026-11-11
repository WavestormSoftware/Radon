cmake_minimum_required(VERSION 3.20)
project(Radon VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The backend is on by default only when Radon is the top-level project (any platform), so a project that
# adds just the dependency-free core does not download and build SDL3.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(_rd_top_level ON)
else()
    set(_rd_top_level OFF)
endif()

# Demos and tests are on by default only when Radon is the top-level project on a desktop.
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR ANDROID OR IOS OR NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(_rd_desktop OFF)
else()
    set(_rd_desktop ON)
endif()
option(RD_BUILD_EXAMPLES "Build the demos" ${_rd_desktop})
option(RD_BUILD_TESTS "Build the tests" ${_rd_desktop})
option(RD_BACKEND_SDL3_GL "Build the SDL3 + OpenGL ES 3 backend (fetches SDL3)" ${_rd_top_level})
option(RD_BUILD_C_API "Build the C API (radon_c.h) into the core library" ${_rd_top_level})
option(RD_BACKEND_RAYLIB "Build the raylib backend and demo (fetches raylib)" OFF)
option(RD_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(RD_INSTALL "Install the core library and a CMake package (find_package(Radon))" ${_rd_desktop})

# ---- core (no platform dependencies) ----
add_library(radon
    src/core/context.cpp src/core/ids.cpp src/core/layout.cpp src/core/window.cpp
    src/core/draw_list.cpp src/core/text.cpp src/core/theme.cpp
    src/core/widgets/basic.cpp src/core/widgets/value.cpp src/core/widgets/popup.cpp
    src/core/widgets/textbox.cpp src/core/widgets/extra.cpp src/core/widgets/icons.cpp
    src/core/widgets/container.cpp src/core/widgets/color.cpp
    src/core/widgets/more.cpp src/core/widgets/overlay.cpp)
add_library(radon::radon ALIAS radon)
if(RD_BUILD_C_API)
    target_sources(radon PRIVATE src/core/radon_c.cpp)
endif()
target_include_directories(radon PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
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
    rd_warnings(radon_sdl3_gl)
endif()

if(RD_BACKEND_RAYLIB)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git GIT_TAG 5.5
        GIT_SHALLOW TRUE FIND_PACKAGE_ARGS 5.5 CONFIG)
    FetchContent_MakeAvailable(raylib)
    add_library(radon_raylib backends/raylib/radon_raylib.cpp)
    add_library(radon::raylib ALIAS radon_raylib)
    target_include_directories(radon_raylib PUBLIC backends/raylib)
    target_link_libraries(radon_raylib PUBLIC radon raylib)
    if(RD_BUILD_EXAMPLES)
        add_executable(radon_raylib_demo examples/raylib/raylib_demo.cpp)
        target_link_libraries(radon_raylib_demo PRIVATE radon_raylib)
        target_compile_definitions(radon_raylib_demo PRIVATE
            RD_FONT_PATH="${CMAKE_SOURCE_DIR}/examples/assets/font.ttf")
    endif()
endif()

if(RD_BUILD_EXAMPLES AND RD_BACKEND_SDL3_GL)
    add_subdirectory(examples)
endif()

# ---- install: the dependency-free core. Use the backend through add_subdirectory / FetchContent. ----
if(RD_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    install(TARGETS radon EXPORT RadonTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY include/radon DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    # The core has no dependencies, so the export file itself is the package config.
    install(EXPORT RadonTargets FILE RadonConfig.cmake NAMESPACE radon::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Radon)
    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/RadonConfigVersion.cmake
        VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/RadonConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Radon)
endif()

if(RD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
