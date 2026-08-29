#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>

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

    windowWidth = width;
    windowHeight = height;

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        windowWidth = mode->width;
        windowHeight = mode->height;
    }

    window = glfwCreateWindow(windowWidth, windowHeight, title.c_str(), NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    if (monitor) {
        glfwMaximizeWindow(window);
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return false;
    }

    glViewport(0, 0, windowWidth, windowHeight);
    glfwSetFramebufferSizeCallback(window, resizeCallback);

    glfwSetWindowUserPointer(window, this);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);

    return true;
}

void World::run() {
    buildWorld();

    float totalTime = 0.0f;
    int frameCount = 0;
    float fpsTimer = 0.0f;

    if (videoExportEnabled) {
        glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);

        double settleDeadline = glfwGetTime() + 0.25;
        while (glfwGetTime() < settleDeadline) {
            glfwPollEvents();
        }

        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        std::string cmd = "./tools/ffmpeg -y -f rawvideo -pix_fmt rgb24 -s " +
                          std::to_string(windowWidth) + "x" + std::to_string(windowHeight) +
                          " -r " + std::to_string(videoFps) +
                          " -i pipe:0 -c:v libx264 -pix_fmt yuv420p -crf 18 " +
                          videoOutputPath;
        std::cout << "ffmpeg: " << cmd << std::endl;
        ffmpegProcess = popen(cmd.c_str(), "w");
        if (!ffmpegProcess) {
            std::cerr << "Failed to launch ./tools/ffmpeg" << std::endl;
            videoExportEnabled = false;
        } else {
            std::cout << "Recording " << windowWidth << "x" << windowHeight
                      << " to " << videoOutputPath << " at " << videoFps << " FPS" << std::endl;
        }
    }

    float lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float dt;
        if (videoExportEnabled) {
            dt = 1.0f / (float)videoFps;
        } else {
            float currentTime = glfwGetTime();
            dt = currentTime - lastTime;
            lastTime = currentTime;
        }
        totalTime += dt;

        processInput(dt);

        timeline.update(dt);
        if (updateCallback) {
            updateCallback(dt, totalTime);
        }

        if (videoExportEnabled && timeline.getDuration() > 0.0f &&
            timeline.getTime() >= timeline.getDuration()) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        updateTriangles();

        fpsTimer += dt;
        frameCount++;
        if (fpsTimer >= 0.3f) {
            std::cout << "FPS: " << frameCount / fpsTimer << std::endl;
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        renderer.render();

        if (videoExportEnabled && ffmpegProcess) {
            std::vector<unsigned char> pixels(windowWidth * windowHeight * 3);
            glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
            int rowBytes = windowWidth * 3;
            std::vector<unsigned char> row(rowBytes);
            for (int y = 0; y < windowHeight / 2; y++) {
                unsigned char* top = pixels.data() + y * rowBytes;
                unsigned char* bot = pixels.data() + (windowHeight - 1 - y) * rowBytes;
                memcpy(row.data(), top, rowBytes);
                memcpy(top, bot, rowBytes);
                memcpy(bot, row.data(), rowBytes);
            }

            fwrite(pixels.data(), 1, pixels.size(), ffmpegProcess);
            fflush(ffmpegProcess);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (ffmpegProcess) {
        pclose(ffmpegProcess);
        std::cout << "Video saved to " << videoOutputPath << std::endl;
        ffmpegProcess = nullptr;
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

Object* World::findObject(const std::string& name) {
    for (Object& object : objects) {
        if (object.name == name) return &object;
    }
    return nullptr;
}

void World::addMaterial(const Material& material) {
    materials.push_back(material);
}

void World::useCanvasCamera() {
    canvasCamera = true;
}

void World::setSceneCamera(const Camera& camera) {
    sceneCameraOverride = camera;
    hasSceneCamera = true;
}

Camera* World::activeCamera() {
    return &renderer.camera;
}

void World::addGlobalLight(const glm::vec3& direction) {
    lightDirection = glm::normalize(direction);
}

Timeline& World::getTimeline() {
    return timeline;
}

void World::onUpdate(std::function<void(float dt, float totalTime)> callback) {
    updateCallback = callback;
}

void World::enableVideoExport(const std::string& outputPath, int fps) {
    videoExportEnabled = true;
    videoOutputPath = outputPath;
    videoFps = fps;
}

void World::buildWorld() {
    std::vector<Triangle> worldTriangles;
    std::vector<TextureData> worldTextures;

    for (Object& object : objects) {
        const Transform& transform = object.transform;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, transform.position);
        model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, transform.scale);

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        int materialBase = (int)materials.size();
        object.materialBase = materialBase;
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
            triangle.layerIndex = object.layer;
            worldTriangles.push_back(triangle);
        }
    }

    if (materials.empty()) {
        materials.push_back({{1.0, 1.0, 1.0, 1.0}, -1});
    }

    maxTriangleCount = worldTriangles.size();

    renderer.lightDirection = lightDirection;
    renderer.load(worldTriangles, materials, worldTextures);
    if (hasSceneCamera) {
        renderer.camera = sceneCameraOverride;
    } else if (canvasCamera) {
        renderer.camera = Camera();
    }
    renderer.applyCameraZoom(renderer.camera.zoom);
    if (windowHeight > 0) {
        renderer.setAspectRatio((float)windowWidth / (float)windowHeight);
    }
}

void World::updateTriangles() {
    std::vector<Triangle> worldTriangles;
    worldTriangles.reserve(maxTriangleCount);

    for (const Object& object : objects) {
        if (!object.visible) continue;

        const Transform& transform = object.transform;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, transform.position);
        model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, transform.scale);

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        size_t triangleLimit = object.triangles.size();
        if (object.drawProgress < 1.0f) {
            float progress = glm::clamp(object.drawProgress, 0.0f, 1.0f);
            triangleLimit = (size_t)(object.triangles.size() * progress + 0.5f);
            triangleLimit -= triangleLimit % 2;
        }

        for (size_t i = 0; i < triangleLimit; i++) {
            Triangle triangle = object.triangles[i];
            triangle.p1 = model * triangle.p1;
            triangle.p2 = model * triangle.p2;
            triangle.p3 = model * triangle.p3;

            triangle.n1 = glm::vec4(normalMatrix * glm::vec3(triangle.n1), 0.0f);
            triangle.n2 = glm::vec4(normalMatrix * glm::vec3(triangle.n2), 0.0f);
            triangle.n3 = glm::vec4(normalMatrix * glm::vec3(triangle.n3), 0.0f);

            triangle.materialIndex = object.materials.empty() ? object.materialIndex : object.materialBase + triangle.materialIndex;
            triangle.layerIndex = object.layer;
            worldTriangles.push_back(triangle);
        }
    }

    if (worldTriangles.size() < maxTriangleCount) {
        Triangle degenerate = {};
        degenerate.p1 = glm::vec4(0.0f);
        degenerate.p2 = glm::vec4(0.0f);
        degenerate.p3 = glm::vec4(0.0f);
        degenerate.n1 = glm::vec4(0, 1, 0, 0);
        degenerate.n2 = glm::vec4(0, 1, 0, 0);
        degenerate.n3 = glm::vec4(0, 1, 0, 0);
        degenerate.materialIndex = 0;
        worldTriangles.resize(maxTriangleCount, degenerate);
    }

    for (Object& object : objects) {
        if (object.opacity == object.lastOpacity) continue;

        for (size_t j = 0; j < object.materials.size(); j++) {
            int materialIdx = object.materialBase + (int)j;
            if (materialIdx >= (int)materials.size()) continue;
            float alpha = materials[materialIdx].colour.a * object.opacity;
            renderer.setMaterialAlpha(materialIdx, alpha);
        }

        object.lastOpacity = object.opacity;
    }

    renderer.updateTriangles(worldTriangles);
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
    World* world = static_cast<World*>(glfwGetWindowUserPointer(window));
    world->windowWidth = width;
    world->windowHeight = height;
    glViewport(0, 0, width, height);
    if (height > 0) {
        world->renderer.setAspectRatio((float)width / (float)height);
    }
}
