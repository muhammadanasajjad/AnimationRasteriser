#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Renderer.h>
#include <iostream>
#include <tracy/Tracy.hpp>

void resizeCallback(GLFWwindow* window, int width, int height) {
    ZoneScoped;
    glViewport(0, 0, width, height);
    // std::cout << "Resize to " << width << " " << height << std::endl;
}

void processInput(GLFWwindow *window) {
    ZoneScoped;
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    ZoneScoped;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, 800, 800);
    glfwSetFramebufferSizeCallback(window, resizeCallback);
    
    Renderer renderer = Renderer();
    renderer.load();
    
    while(!glfwWindowShouldClose(window)) {
        processInput(window);
        
        glClear(GL_COLOR_BUFFER_BIT);
        renderer.render();
        
        FrameMark;
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    renderer.offload();
    glfwTerminate();
    
    return 0;
}
