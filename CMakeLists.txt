cmake_minimum_required(VERSION 3.16)

project(AnimationRasteriser)

file(GLOB_RECURSE SRC_FILES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE GLAD_SRC_FILES
    "${CMAKE_SOURCE_DIR}/glad/src/*.c"
)

add_executable(AnimationRasteriser ${SRC_FILES} ${GLAD_SRC_FILES})

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

find_path(GLM_INCLUDE_DIR glm/glm.hpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/glad/include
    ${CMAKE_SOURCE_DIR}/vendor
    ${GLM_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}    glfw
    OpenGL::GL
)
