#pragma once

#include <glm/glm.hpp>

struct ProjectedTriangle {
    glm::vec2 p1;
    glm::vec2 p2;
    glm::vec2 p3;

    glm::vec2 minBounds;
    glm::vec2 maxBounds;
    
    glm::vec2 padding;
};

struct Triangle {
    glm::vec4 p1;
    glm::vec4 p2;
    glm::vec4 p3;
};
