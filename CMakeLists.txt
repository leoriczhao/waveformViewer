cmake_minimum_required(VERSION 3.16)
project(waveform_viewer VERSION 0.1.0 LANGUAGES C CXX)

option(WV_OFFICIAL_BUILD "Official (release) build settings" OFF)
option(WV_SHARED_LIB "Build shared library" OFF)
option(WV_WERROR "Treat warnings as errors" OFF)
option(WV_ENABLE_GL "Enable OpenGL backend if available" ON)
option(WV_ENABLE_EXAMPLES "Build example applications" ON)
option(WV_ENABLE_TESTS "Build tests" ON)

if(WV_OFFICIAL_BUILD)
    if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Core library sources (platform-independent)
set(WAVEFORM_CORE_SOURCES
    src/canvas.cpp
    src/draw_pass.cpp
    src/gpu_device.cpp
    src/pixmap.cpp
    src/raster_device.cpp
    src/recording.cpp
    src/surface.cpp
    src/surface_raster.cpp
    src/surface_recording.cpp
    src/waveform_viewer.cpp
    src/vcd_parser.cpp
    src/glyph_cache.cpp
)

if(WV_SHARED_LIB)
    set(WV_LIBRARY_TYPE SHARED)
else()
    set(WV_LIBRARY_TYPE STATIC)
endif()

add_library(waveform_core ${WV_LIBRARY_TYPE} ${WAVEFORM_CORE_SOURCES})
target_include_directories(waveform_core PUBLIC include third_party)

if(WV_WERROR)
    target_compile_options(waveform_core PRIVATE -Werror)
endif()

# Optional: OpenGL backend
find_package(OpenGL)
if(WV_ENABLE_GL AND OpenGL_FOUND)
    set(GL_BACKEND_SOURCES
        glx/gl_context.cpp
        glx/surface_gpu_gl.cpp
        glx/glad/glad.c
    )
    add_library(waveform_backend_glx ${WV_LIBRARY_TYPE} ${GL_BACKEND_SOURCES})
    target_include_directories(waveform_backend_glx PUBLIC include glx glx/glad)
    target_link_libraries(waveform_backend_glx PUBLIC waveform_core OpenGL::GL ${CMAKE_DL_LIBS})
    target_compile_definitions(waveform_backend_glx PUBLIC WAVEFORM_HAS_GL=1)
    if(WV_WERROR)
        target_compile_options(waveform_backend_glx PRIVATE -Werror)
    endif()
endif()

# Convenience: combined library
add_library(waveform_viewer INTERFACE)
target_link_libraries(waveform_viewer INTERFACE waveform_core)
if(WV_ENABLE_GL AND OpenGL_FOUND)
    target_link_libraries(waveform_viewer INTERFACE waveform_backend_glx)
    target_compile_definitions(waveform_viewer INTERFACE WAVEFORM_HAS_GL=1)
else()
    target_compile_definitions(waveform_viewer INTERFACE WAVEFORM_HAS_GL=0)
endif()

# Example application (uses XCB + optional GLX for windowing)
if(WV_ENABLE_EXAMPLES)
    find_package(X11)
    add_executable(waveform_example
        examples/waveform_example.cpp
    )
    target_link_libraries(waveform_example PRIVATE waveform_viewer)
    if(X11_FOUND)
        target_link_libraries(waveform_example PRIVATE xcb X11)
    endif()
endif()

if(WV_ENABLE_TESTS)
    find_package(GTest REQUIRED)
    add_executable(waveform_tests
        tests/test_main.cpp
    )
    target_link_libraries(waveform_tests PRIVATE waveform_viewer GTest::gtest GTest::gmock GTest::gtest_main)
endif()
