#include <iostream>
#include <fstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <World.h>

bool World::firstMouse = true;
double World::lastMouseX = 0.0;
double World::lastMouseY = 0.0;

World::World() : window(nullptr) {}

World::~World() {
    cleanup();
}

bool World::init(int width, int height, const std::string& title) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return false;
    }

    glViewport(0, 0, width, height);
    glfwSetFramebufferSizeCallback(window, resizeCallback);

    glfwSetWindowUserPointer(window, this);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);

    return true;
}

void World::run() {
    buildWorld();

    float lastTime = glfwGetTime();
    int frameCount = 0;
    float fpsTimer = 0.0f;
    int captureFrames = 30;
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        processInput(dt);

        fpsTimer += dt;
        frameCount++;
        if (fpsTimer >= 0.3f) {
            std::cout << "FPS: " << frameCount / fpsTimer << std::endl;
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        renderer.render();

        if (--captureFrames == 0) {
            std::vector<unsigned char> pixels(800 * 800 * 3);
            glReadPixels(0, 0, 800, 800, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
            std::ofstream out("/tmp/render_before.ppm", std::ios::binary);
            out << "P6\n" << 800 << " " << 800 << "\n255\n";
            out.write((const char*)pixels.data(), (std::streamsize)pixels.size());
            std::cout << "captured" << std::endl;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void World::cleanup() {
    if (window == nullptr) return;
    renderer.offload();
    glfwDestroyWindow(window);
    window = nullptr;
    glfwTerminate();
}

Object& World::addObject(const Object& object) {
    objects.push_back(object);
    return objects.back();
}

void World::addMaterial(const Material& material) {
    materials.push_back(material);
}

void World::addGlobalLight(const glm::vec3& direction) {
    lightDirection = glm::normalize(direction);
}

void World::buildWorld() {
    std::vector<Triangle> worldTriangles;
    std::vector<TextureData> worldTextures;

    for (const Object& object : objects) {
        const Transform& transform = object.transform;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, transform.position);
        model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, transform.scale);

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        int materialBase = (int)materials.size();
        for (const Material& material : object.materials) {
            materials.push_back(material);
        }
        for (const TextureData& tex : object.textures) {
            worldTextures.push_back(tex);
        }

        for (Triangle triangle : object.triangles) {
            triangle.p1 = model * triangle.p1;
            triangle.p2 = model * triangle.p2;
            triangle.p3 = model * triangle.p3;

            triangle.n1 = glm::vec4(normalMatrix * glm::vec3(triangle.n1), 0.0f);
            triangle.n2 = glm::vec4(normalMatrix * glm::vec3(triangle.n2), 0.0f);
            triangle.n3 = glm::vec4(normalMatrix * glm::vec3(triangle.n3), 0.0f);

            triangle.materialIndex = object.materials.empty() ? object.materialIndex : materialBase + triangle.materialIndex;
            worldTriangles.push_back(triangle);
        }
    }

    if (materials.empty()) {
        materials.push_back({{1.0, 1.0, 1.0, 1.0}, -1});
    }

    renderer.lightDirection = lightDirection;
    renderer.load(worldTriangles, materials, worldTextures);
}

void World::processInput(float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    Camera* camera = &renderer.camera;
    float amount = camera->moveSpeed * dt;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->moveForward(amount);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->moveBackward(amount);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->moveLeft(amount);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->moveRight(amount);
}

void World::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    World* world = static_cast<World*>(glfwGetWindowUserPointer(window));

    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float dx = xpos - lastMouseX;
    float dy = ypos - lastMouseY;
    lastMouseX = xpos;
    lastMouseY = ypos;

    world->renderer.camera.look(dx, dy);
}

void World::resizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
