cmake_minimum_required(VERSION 3.16)
project(component_framework VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(component_core INTERFACE)
target_include_directories(component_core INTERFACE include)

option(BUILD_OPENGL_SURFACE "Build OpenGL Surface implementation" ON)
option(BUILD_EXAMPLES "Build example applications" ON)

if(BUILD_OPENGL_SURFACE)
    find_package(OpenGL REQUIRED)
    add_library(component_opengl STATIC platform/opengl/opengl_surface.cpp)
    target_link_libraries(component_opengl PUBLIC component_core OpenGL::GL)
    target_include_directories(component_opengl PUBLIC include)
endif()

if(BUILD_EXAMPLES)
    find_package(SDL2 REQUIRED)
    add_executable(waveform_example examples/waveform_example.cpp)
    target_link_libraries(waveform_example PRIVATE component_opengl SDL2::SDL2)
endif()
