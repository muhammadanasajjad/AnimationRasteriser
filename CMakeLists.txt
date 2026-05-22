cmake_minimum_required(VERSION 3.20)

project(AnimationRasteriser CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h src/*.hpp)

add_executable(AnimationRasteriser ${SOURCES})

target_include_directories(AnimationRasteriser PRIVATE src)

target_link_libraries(AnimationRasteriser
    glfw
    OpenGL::GL
)
