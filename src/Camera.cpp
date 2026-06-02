#include <Camera.h>
#include <glm/glm.hpp>
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

Camera::Camera(glm::vec3 position, glm::vec3 forward) {
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    this->position = position;
    this->forward = glm::normalize(forward);
    up = glm::vec3(0, 0, 1);
}

Camera::Camera() {
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    position = {0, 0, 0};
    forward = {0, 1, 0};
    up = {0, 0, 1};
}
