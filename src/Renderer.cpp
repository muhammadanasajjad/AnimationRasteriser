#include <glad/glad.h>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <Renderer.h>
#include <FileLoader.h>
#include <Camera.h>
#include <RendererStructs.h>
#include <glm/gtc/matrix_transform.hpp>
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void checkShaderCompilation(unsigned int shader, std::string errorMessage) {
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << errorMessage << "\n" << infoLog << std::endl;
    }
}

void checkShaderProgramLink(unsigned int shaderProgram, std::string errorMessage) {
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    
    if(!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << errorMessage << "\n" << infoLog << std::endl;
    }
}

void Renderer::load() {
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    FileLoader fileLoader = FileLoader();
    
    // projection compute setup
    fileLoader.loadFile("./shaders/projection.glsl");
    std::string projectionShaderSource = fileLoader.getFileAsString();
    const char* projectionShaderChars = projectionShaderSource.c_str();
    
    projectionShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(projectionShader, 1, &projectionShaderChars, NULL);
    glCompileShader(projectionShader);
    
    checkShaderCompilation(projectionShader, "Error compiling projection shader");
    
    projectionProgram = glCreateProgram();
    glAttachShader(projectionProgram, projectionShader);
    glLinkProgram(projectionProgram);
    
    checkShaderProgramLink(projectionProgram, "Error linking projection shader program");
    
    // vertex shader load
    fileLoader.loadFile("./shaders/screenVertex.vert");
    std::string vertexShaderSource = fileLoader.getFileAsString();
    const char* vertexShaderChars = vertexShaderSource.c_str();
    
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    
    glShaderSource(vertexShader, 1, &vertexShaderChars, NULL);
    glCompileShader(vertexShader);
    
    checkShaderCompilation(vertexShader, "Error compiling vertex shader");
    
    // fragment shader load
    fileLoader.loadFile("./shaders/screenFragment.frag");
    std::string fragmentShaderSource = fileLoader.getFileAsString();
    const char* fragmentShaderChars = fragmentShaderSource.c_str();
    
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(fragmentShader, 1, &fragmentShaderChars, NULL);
    glCompileShader(fragmentShader);
    
    checkShaderCompilation(fragmentShader, "Error compiling fragment shader");
    
    // Link vertex and fragment shader
    mainShaderProgram = glCreateProgram();
    glAttachShader(mainShaderProgram, vertexShader);
    glAttachShader(mainShaderProgram, fragmentShader);
    glLinkProgram(mainShaderProgram);
    
    checkShaderProgramLink(mainShaderProgram, "Error linking main shader program");
    
    camera = Camera({-10, 0, 0}, {+1, 0, 0});
    
    glUseProgram(mainShaderProgram);
    
    std::vector<float> screenVertices = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, screenVertices.size() * sizeof(float), screenVertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::vec3 cubeOffset(-7.5f, 0.0f, 0.0f);
    const std::vector<Triangle> cube = {
        // Front (z = 0.5)
        {rot * glm::vec4(-0.5, -0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5, -0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
        {rot * glm::vec4(-0.5, -0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
        // Back  (z = -0.5)
        {rot * glm::vec4( 0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
        {rot * glm::vec4( 0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
        // Right (x = 0.5)
        {rot * glm::vec4( 0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
        {rot * glm::vec4( 0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
        // Left  (x = -0.5)
        {rot * glm::vec4(-0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
        {rot * glm::vec4(-0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
        // Top   (y = 0.5)
        {rot * glm::vec4(-0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
        {rot * glm::vec4(-0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
        // Bottom (y = -0.5)
        {rot * glm::vec4(-0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
        {rot * glm::vec4(-0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f)},
    };
    
    glGenBuffers(1, &worldTrianglesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, worldTrianglesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 cube.size() * sizeof(Triangle),
                 cube.data(),
                 GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, worldTrianglesSSBO);
    
    glUseProgram(projectionProgram);
    worldTriangleCountLoc = glGetUniformLocation(projectionProgram, "triangleCount");
    worldTriangleCount = cube.size();
    glUniform1i(worldTriangleCountLoc, worldTriangleCount);
    
    glGenBuffers(1, &projectedTrianglesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedTrianglesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 worldTriangleCount * sizeof(ProjectedTriangle),
                 NULL,
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, projectedTrianglesSSBO);
    
    glUseProgram(projectionProgram);
    camPosLoc = glGetUniformLocation(projectionProgram, "cameraPosition");
    camFwdLoc = glGetUniformLocation(projectionProgram, "cameraForward");
    camUpLoc = glGetUniformLocation(projectionProgram, "cameraUp");
    
    glUseProgram(mainShaderProgram);
    projectedTriangleCountLoc = glGetUniformLocation(mainShaderProgram, "projectedTriangleCount");
    glUniform1i(projectedTriangleCountLoc, worldTriangleCount);
}

void Renderer::render() {
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    frame++;
    
    glUseProgram(projectionProgram);
    glUniform3fv(camPosLoc, 1, &camera.position[0]);
    glUniform3fv(camFwdLoc, 1, &camera.forward[0]);
    glUniform3fv(camUpLoc, 1, &camera.up[0]);
    glDispatchCompute(worldTriangleCount, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
    glUseProgram(mainShaderProgram);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedTrianglesSSBO);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer::offload() {
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}
