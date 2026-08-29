#include <Camera.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

Camera::Camera(glm::vec3 position, glm::vec3 forward) {
    this->position = position;
    this->forward = glm::normalize(forward);
    up = glm::vec3(0, 0, 1);
    yaw = atan2(this->forward.y, this->forward.x);
    pitch = atan2(this->forward.z, sqrt(this->forward.x * this->forward.x + this->forward.y * this->forward.y));
    roll = 0.0f;
    updateVectors();
}

Camera::Camera() {
    position = {0, 0, 0};
    forward = {1, 0, 0};
    up = {0, 0, 1};
    yaw = 0.0f;
    pitch = 0.0f;
    roll = 0.0f;
    updateVectors();
}

void Camera::updateVectors() {
    glm::vec3 worldUp = glm::vec3(0, 0, 1);
    glm::vec3 horizontal = glm::vec3(glm::cos(yaw), glm::sin(yaw), 0.0f);
    forward = glm::normalize(horizontal * glm::cos(pitch) + worldUp * glm::sin(pitch));
    glm::vec3 nominalRight = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 nominalUp = glm::normalize(glm::cross(nominalRight, forward));
    if (roll != 0.0f) {
        float cosR = std::cos(roll);
        float sinR = std::sin(roll);
        right = glm::normalize(nominalRight * cosR + nominalUp * sinR);
        up = glm::normalize(glm::cross(right, forward));
    } else {
        right = nominalRight;
        up = nominalUp;
    }
}

void Camera::lookAt(const glm::vec3& target) {
    glm::vec3 dir = glm::normalize(target - position);
    yaw = atan2(dir.y, dir.x);
    pitch = asin(glm::clamp(dir.z, -1.0f, 1.0f));
    roll = 0.0f;
    updateVectors();
}

void Camera::setOrientation(const glm::vec3& dir, float bank) {
    glm::vec3 f = glm::normalize(dir);
    yaw = atan2(f.y, f.x);
    pitch = asin(glm::clamp(f.z, -1.0f, 1.0f));
    roll = bank;
    forward = f;
    updateVectors();
}

void Camera::look(float dx, float dy) {
    yaw -= dx * lookSensitivity;
    pitch += dy * lookSensitivity;

    float maxPitch = glm::radians(89.0f);
    pitch = glm::clamp(pitch, -maxPitch, maxPitch);

    updateVectors();
}

void Camera::moveForward(float amount) {
    position += forward * amount;
}

void Camera::moveBackward(float amount) {
    moveForward(-amount);
}

void Camera::moveLeft(float amount) {
    position -= right * amount;
}

void Camera::moveRight(float amount) {
    moveLeft(-amount);
}
