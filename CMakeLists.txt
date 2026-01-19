cmake_minimum_required(VERSION 3.16)
project(waveform_viewer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Core library (no rendering backend)
add_library(waveform_core STATIC
    src/waveform_viewer.cpp
    src/vcd_parser.cpp
)
target_include_directories(waveform_core PUBLIC include)

# Backend selection
set(WAVEFORM_BACKEND "xcb" CACHE STRING "Rendering backend (xcb, gl)")
set_property(CACHE WAVEFORM_BACKEND PROPERTY STRINGS xcb gl)

if(WAVEFORM_BACKEND STREQUAL "xcb")
    add_library(waveform_backend STATIC
        xcb/xcb_surface.cpp
    )
    target_include_directories(waveform_backend PUBLIC include xcb)
    target_link_libraries(waveform_backend PUBLIC waveform_core xcb)
elseif(WAVEFORM_BACKEND STREQUAL "gl")
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(X11 REQUIRED)
    
    add_library(waveform_backend STATIC
        gl/gl_surface.cpp
    )
    target_include_directories(waveform_backend PUBLIC include gl)
    target_link_libraries(waveform_backend PUBLIC waveform_core OpenGL::GL GLEW::GLEW X11::X11)
endif()

# Unified library name
add_library(waveform_viewer INTERFACE)
target_link_libraries(waveform_viewer INTERFACE waveform_backend)

# Examples
option(BUILD_EXAMPLES "Build example applications" ON)
if(BUILD_EXAMPLES)
    if(WAVEFORM_BACKEND STREQUAL "xcb")
        add_executable(waveform_example examples/waveform_example.cpp)
        target_link_libraries(waveform_example PRIVATE waveform_viewer)
    elseif(WAVEFORM_BACKEND STREQUAL "gl")
        add_executable(gl_example examples/gl_example.cpp)
        target_link_libraries(gl_example PRIVATE waveform_viewer)
    endif()
endif()

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    add_executable(waveform_tests tests/test_main.cpp)
    target_link_libraries(waveform_tests PRIVATE waveform_core GTest::gtest GTest::gmock GTest::gtest_main)
endif()
