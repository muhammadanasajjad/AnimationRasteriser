#pragma once

#include <glm/glm.hpp>
#include <vector>

inline constexpr size_t PROJECTED_TRIANGLE_GPU_STRIDE = 128;

struct ProjectedTriangle {
    glm::vec2 p1;
    glm::vec2 p2;
    glm::vec2 p3;

    glm::vec2 uv1;
    glm::vec2 uv2;
    glm::vec2 uv3;

    glm::vec4 n1;
    glm::vec4 n2;
    glm::vec4 n3;

    float depths[3];

    int materialIndex;
    int layerIndex;
    int gpuPadding0;
    int gpuPadding1;
    int gpuPadding2;
};

struct Triangle {
    glm::vec4 p1;
    glm::vec4 p2;
    glm::vec4 p3;
    glm::vec4 n1;
    glm::vec4 n2;
    glm::vec4 n3;
    glm::vec2 uv1;
    glm::vec2 uv2;
    glm::vec2 uv3;
    int materialIndex;
    int layerIndex = 0;
};

struct Material {
    glm::vec4 colour;
    int textureIndex = -1;
    glm::vec3 padding;
};

struct TextureData {
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    int channels = 0;
};
