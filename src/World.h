#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <Renderer.h>
#include <RendererStructs.h>

struct GLFWwindow;

struct Transform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

struct Object {
    std::vector<Triangle> triangles;
    Transform transform;
    int materialIndex = 0;
    std::vector<Material> materials;
    std::vector<TextureData> textures;
};

class World {
    public:
        World();
        ~World();

        bool init(int width, int height, const std::string& title);
        void run();
        void cleanup();

        Object& addObject(const Object& object);
        void addMaterial(const Material& material);
        void addGlobalLight(const glm::vec3& direction);

    private:
        void buildWorld();
        void processInput(float dt);

        static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
        static void resizeCallback(GLFWwindow* window, int width, int height);

        Renderer renderer;
        GLFWwindow* window;
        std::vector<Object> objects;
        std::vector<Material> materials;
        glm::vec3 lightDirection = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));

        static bool firstMouse;
        static double lastMouseX;
        static double lastMouseY;
};
