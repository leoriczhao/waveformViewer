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

target_link_libraries(waveform_core PUBLIC)

add_library(waveform_backend_xcb STATIC
    xcb/xcb_surface.cpp
)
target_include_directories(waveform_backend_xcb PUBLIC include xcb)
target_link_libraries(waveform_backend_xcb PUBLIC waveform_core xcb)

find_package(OpenGL)
find_package(X11)
if(OpenGL_FOUND AND X11_FOUND)
    add_library(waveform_backend_glx STATIC
        glx/gl_surface.cpp
        glx/glad/glad.c
    )
    target_include_directories(waveform_backend_glx PUBLIC include glx glx/glad)
    target_link_libraries(waveform_backend_glx PUBLIC waveform_core OpenGL::GL X11)
endif()

option(BUILD_EXAMPLES "Build example applications" ON)
if(BUILD_EXAMPLES)
    add_executable(waveform_example
        examples/waveform_example.cpp
    )
    target_link_libraries(waveform_example PRIVATE waveform_backend_xcb)
    
    if(TARGET waveform_backend_glx)
        add_executable(gl_waveform_example
            examples/gl_waveform_example.cpp
        )
        target_link_libraries(gl_waveform_example PRIVATE waveform_backend_glx)
    endif()
endif()

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    add_executable(waveform_tests
        tests/test_main.cpp
    )
    target_link_libraries(waveform_tests PRIVATE waveform_core GTest::gtest GTest::gmock GTest::gtest_main)
endif()
