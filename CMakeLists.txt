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

set(WAVEFORM_SOURCES
    src/canvas.cpp
    src/gpu_device.cpp
    src/recording.cpp
    src/surface.cpp
    src/surface_recording.cpp
    src/waveform_viewer.cpp
    src/vcd_parser.cpp
    src/glyph_cache.cpp
    xcb/xcb_raster_device.cpp
    xcb/surface_raster_xcb.cpp
)

if(WV_SHARED_LIB)
    set(WV_LIBRARY_TYPE SHARED)
else()
    set(WV_LIBRARY_TYPE STATIC)
endif()

find_package(OpenGL)
find_package(X11)
if(WV_ENABLE_GL AND OpenGL_FOUND AND X11_FOUND)
    list(APPEND WAVEFORM_SOURCES
        glx/gl_context.cpp
        glx/surface_gpu_gl.cpp
        glx/glad/glad.c
    )
endif()

add_library(waveform_viewer ${WV_LIBRARY_TYPE} ${WAVEFORM_SOURCES})
target_include_directories(waveform_viewer PUBLIC include third_party xcb)
target_link_libraries(waveform_viewer PUBLIC xcb)

if(WV_WERROR)
    target_compile_options(waveform_viewer PRIVATE -Werror)
endif()

if(WV_ENABLE_GL AND OpenGL_FOUND AND X11_FOUND)
    target_include_directories(waveform_viewer PUBLIC glx glx/glad)
    target_link_libraries(waveform_viewer PUBLIC OpenGL::GL X11)
    target_compile_definitions(waveform_viewer PUBLIC WAVEFORM_HAS_GL=1)
else()
    target_compile_definitions(waveform_viewer PUBLIC WAVEFORM_HAS_GL=0)
endif()

if(WV_ENABLE_EXAMPLES)
    add_executable(waveform_example
        examples/waveform_example.cpp
    )
    target_link_libraries(waveform_example PRIVATE waveform_viewer)
endif()

if(WV_ENABLE_TESTS)
    find_package(GTest REQUIRED)
    add_executable(waveform_tests
        tests/test_main.cpp
    )
    target_link_libraries(waveform_tests PRIVATE waveform_viewer GTest::gtest GTest::gmock GTest::gtest_main)
endif()
