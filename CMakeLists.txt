cmake_minimum_required(VERSION 3.16)
project(waveform_viewer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(waveform_core STATIC
    src/waveform_viewer.cpp
    src/vcd_parser.cpp
)
target_include_directories(waveform_core PUBLIC include)

# -------------------------
# Backends
# -------------------------
option(BUILD_XCB_BACKEND "Build the XCB (CPU) backend" ON)
option(BUILD_GLX_BACKEND "Build the GLX/OpenGL backend" ON)
option(BUILD_MACOS_BACKEND "Build the macOS/CoreGraphics (CPU) backend" ON)

include(CheckIncludeFileCXX)
check_include_file_cxx("xcb/xcb.h" HAVE_XCB_H)
check_include_file_cxx("X11/Xlib.h" HAVE_X11_XLIB_H)
check_include_file_cxx("GL/gl.h" HAVE_GL_GL_H)

if(BUILD_XCB_BACKEND AND HAVE_XCB_H)
    add_library(waveform_backend_xcb STATIC
        xcb/xcb_surface.cpp
    )
    target_include_directories(waveform_backend_xcb PUBLIC include xcb)
    target_link_libraries(waveform_backend_xcb PUBLIC waveform_core xcb)
else()
    message(STATUS "Skipping waveform_backend_xcb (missing xcb/xcb.h or BUILD_XCB_BACKEND=OFF)")
endif()

find_package(OpenGL)
find_package(X11)
if(BUILD_GLX_BACKEND AND OpenGL_FOUND AND X11_FOUND AND HAVE_X11_XLIB_H AND HAVE_GL_GL_H)
    add_library(waveform_backend_glx STATIC
        glx/gl_surface.cpp
        glx/glad/glad.c
    )
    target_include_directories(waveform_backend_glx PUBLIC include glx glx/glad)
    target_link_libraries(waveform_backend_glx PUBLIC waveform_core OpenGL::GL X11)
else()
    message(STATUS "Skipping waveform_backend_glx (missing headers/deps or BUILD_GLX_BACKEND=OFF)")
endif()

if(APPLE AND BUILD_MACOS_BACKEND)
    add_library(waveform_backend_macos STATIC
        macos/MacSurface.mm
    )
    target_include_directories(waveform_backend_macos PUBLIC include macos)

    find_library(FW_COREGRAPHICS CoreGraphics)
    find_library(FW_CORETEXT CoreText)
    find_library(FW_COREFOUNDATION CoreFoundation)

    if(NOT FW_COREGRAPHICS OR NOT FW_CORETEXT OR NOT FW_COREFOUNDATION)
        message(FATAL_ERROR "Missing CoreGraphics/CoreText/CoreFoundation frameworks")
    endif()

    target_link_libraries(waveform_backend_macos PUBLIC waveform_core "${FW_COREGRAPHICS}" "${FW_CORETEXT}" "${FW_COREFOUNDATION}")
else()
    if(NOT APPLE)
        message(STATUS "Skipping waveform_backend_macos (not on Apple platform)")
    else()
        message(STATUS "Skipping waveform_backend_macos (BUILD_MACOS_BACKEND=OFF)")
    endif()
endif()

# -------------------------
# Examples
# -------------------------
option(BUILD_EXAMPLES "Build example applications" ON)
if(BUILD_EXAMPLES)
    if(TARGET waveform_backend_xcb)
        add_executable(waveform_example
            examples/waveform_example.cpp
        )
        target_link_libraries(waveform_example PRIVATE waveform_backend_xcb)
    else()
        message(STATUS "Skipping waveform_example (waveform_backend_xcb not available)")
    endif()

    if(TARGET waveform_backend_glx)
        add_executable(gl_waveform_example
            examples/gl_waveform_example.cpp
        )
        target_link_libraries(gl_waveform_example PRIVATE waveform_backend_glx)
    endif()

    if(TARGET waveform_backend_macos)
        add_executable(macos_render_example
            examples/macos_render_example.mm
        )

        find_library(FW_IMAGEIO ImageIO)
        find_library(FW_COREGRAPHICS CoreGraphics)
        find_library(FW_CORETEXT CoreText)
        find_library(FW_COREFOUNDATION CoreFoundation)

        target_link_libraries(macos_render_example PRIVATE waveform_backend_macos "${FW_IMAGEIO}" "${FW_COREGRAPHICS}" "${FW_CORETEXT}" "${FW_COREFOUNDATION}")
    endif()
endif()

# -------------------------
# Tests
# -------------------------
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    add_executable(waveform_tests
        tests/test_main.cpp
    )
    target_link_libraries(waveform_tests PRIVATE waveform_core GTest::gtest GTest::gmock GTest::gtest_main)
endif()
