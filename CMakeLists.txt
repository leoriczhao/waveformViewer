cmake_minimum_required(VERSION 3.16)
project(waveform_viewer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_EXAMPLES "Build example applications" ON)
if(BUILD_EXAMPLES)
    add_executable(waveform_example
        examples/waveform_example.cpp
        src/waveform_viewer.cpp
        src/vcd_parser.cpp
        xcb/xcb_surface.cpp
    )
    target_include_directories(waveform_example PRIVATE include xcb)
    target_link_libraries(waveform_example PRIVATE xcb)
    
    find_package(OpenGL)
    find_package(X11)
    if(OpenGL_FOUND AND X11_FOUND)
        add_executable(gl_waveform_example
            examples/gl_waveform_example.cpp
            src/waveform_viewer.cpp
            src/vcd_parser.cpp
            glx/gl_surface.cpp
            glx/glad/glad.c
        )
        target_include_directories(gl_waveform_example PRIVATE include glx glx/glad)
        target_link_libraries(gl_waveform_example PRIVATE OpenGL::GL X11)
    endif()
endif()

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    add_executable(waveform_tests
        tests/test_main.cpp
        src/waveform_viewer.cpp
        src/vcd_parser.cpp
    )
    target_include_directories(waveform_tests PRIVATE include)
    target_link_libraries(waveform_tests PRIVATE GTest::gtest GTest::gmock GTest::gtest_main)
endif()
