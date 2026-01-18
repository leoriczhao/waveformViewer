cmake_minimum_required(VERSION 3.16)
project(waveform_viewer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(waveform_viewer STATIC
    src/waveform_viewer.cpp
    src/vcd_parser.cpp
    xcb/xcb_surface.cpp
)
target_include_directories(waveform_viewer PUBLIC include PRIVATE xcb)
target_link_libraries(waveform_viewer PUBLIC xcb)

option(BUILD_EXAMPLES "Build example applications" ON)
if(BUILD_EXAMPLES)
    add_executable(waveform_example examples/waveform_example.cpp)
    target_link_libraries(waveform_example PRIVATE waveform_viewer)
    target_include_directories(waveform_example PRIVATE xcb)
endif()

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    add_executable(waveform_tests tests/test_main.cpp)
    target_link_libraries(waveform_tests PRIVATE waveform_viewer GTest::gtest GTest::gmock GTest::gtest_main)
    target_include_directories(waveform_tests PRIVATE xcb)
endif()
