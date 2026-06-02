#pragma once

#include <glm/glm.hpp>

class Camera {
    public:
        glm::vec3 position;
        glm::vec3 forward;
        glm::vec3 up;
        Camera();
        Camera(glm::vec3 position, glm::vec3 forward);
};
