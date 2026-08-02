#include <Camera.h>
#include <glm/glm.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 forward) {
    this->position = position;
    this->forward = glm::normalize(forward);
    up = glm::vec3(0, 0, 1);
}

Camera::Camera() {
    position = {0, 0, 0};
    forward = {0, 1, 0};
    up = {0, 0, 1};
}
