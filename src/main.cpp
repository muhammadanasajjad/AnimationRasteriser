#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Renderer.h>
#include <iostream>

static bool firstMouse = true;
static double lastMouseX = 0.0;
static double lastMouseY = 0.0;

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float dx = xpos - lastMouseX;
    float dy = ypos - lastMouseY;
    lastMouseX = xpos;
    lastMouseY = ypos;

    renderer->camera.look(dx, dy);
}

void processInput(GLFWwindow *window, Renderer* renderer, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    Camera* camera = &renderer->camera;
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

void resizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    // std::cout << "Resize to " << width << " " << height << std::endl;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(800, 800, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, 800, 800);
    glfwSetFramebufferSizeCallback(window, resizeCallback);
    
    Renderer renderer = Renderer();
    renderer.load();

    glfwSetWindowUserPointer(window, &renderer);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    
    float lastTime = glfwGetTime();
    int frameCount = 0;
    float fpsTimer = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        processInput(window, &renderer, dt);

        fpsTimer += dt;
        frameCount++;
        if (fpsTimer >= 0.3f) {
            std::cout << "FPS: " << frameCount / fpsTimer << std::endl;
            frameCount = 0;
            fpsTimer = 0.0f;
        }
        
        glClear(GL_COLOR_BUFFER_BIT);
        renderer.render();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    renderer.offload();
    glfwTerminate();
    
    return 0;
}
