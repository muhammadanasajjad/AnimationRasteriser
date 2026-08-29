#pragma once

#include <glm/glm.hpp>

class Camera {
    public:
        glm::vec3 position;
        glm::vec3 forward;
        glm::vec3 up;
        glm::vec3 right;
        float yaw = 0.0f;
        float pitch = 0.0f;
        float roll = 0.0f;
        float zoom = 1.0f;
        float moveSpeed = 30.0f;
        float lookSensitivity = 0.003f;
        Camera();
        Camera(glm::vec3 position, glm::vec3 forward);
        void updateVectors();
        void look(float dx, float dy);
        void moveForward(float amount);
        void moveBackward(float amount);
        void moveLeft(float amount);
        void moveRight(float amount);
        void lookAt(const glm::vec3& target);
        void setOrientation(const glm::vec3& forward, float bank);
};
