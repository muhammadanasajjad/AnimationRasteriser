#include <glad/glad.h>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <Renderer.h>
#include <FileLoader.h>
#include <Camera.h>
#include <RendererStructs.h>
#include <tracy/Tracy.hpp>

void checkShaderCompilation(unsigned int shader, std::string errorMessage) {
    ZoneScoped;
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << errorMessage << "\n" << infoLog << std::endl;
    }
}

void checkShaderProgramLink(unsigned int shaderProgram, std::string errorMessage) {
    ZoneScoped;
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    
    if(!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << errorMessage << "\n" << infoLog << std::endl;
    }
}

void Renderer::load() {
    ZoneScoped;
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
    
    camera = Camera({-2, -2, 0}, {+1, +1, 0});
    
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
    
    const std::vector<Triangle> cube = {
        // Front (z = 0.5)
        {{-0.5, -0.5, 0.5}, { 0.5, -0.5, 0.5}, { 0.5,  0.5, 0.5}},
        {{-0.5, -0.5, 0.5}, { 0.5,  0.5, 0.5}, {-0.5,  0.5, 0.5}},
        // Back  (z = -0.5)
        {{ 0.5, -0.5, -0.5}, {-0.5, -0.5, -0.5}, {-0.5,  0.5, -0.5}},
        {{ 0.5, -0.5, -0.5}, {-0.5,  0.5, -0.5}, { 0.5,  0.5, -0.5}},
        // Right (x = 0.5)
        {{ 0.5, -0.5, -0.5}, { 0.5, -0.5,  0.5}, { 0.5,  0.5,  0.5}},
        {{ 0.5, -0.5, -0.5}, { 0.5,  0.5,  0.5}, { 0.5,  0.5, -0.5}},
        // Left  (x = -0.5)
        {{-0.5, -0.5,  0.5}, {-0.5, -0.5, -0.5}, {-0.5,  0.5, -0.5}},
        {{-0.5, -0.5,  0.5}, {-0.5,  0.5, -0.5}, {-0.5,  0.5,  0.5}},
        // Top   (y = 0.5)
        {{-0.5,  0.5,  0.5}, { 0.5,  0.5,  0.5}, { 0.5,  0.5, -0.5}},
        {{-0.5,  0.5,  0.5}, { 0.5,  0.5, -0.5}, {-0.5,  0.5, -0.5}},
        // Bottom (y = -0.5)
        {{-0.5, -0.5, -0.5}, { 0.5, -0.5, -0.5}, { 0.5, -0.5,  0.5}},
        {{-0.5, -0.5, -0.5}, { 0.5, -0.5,  0.5}, {-0.5, -0.5,  0.5}},
    };
    
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    int n = 1;
    std::vector<ProjectedTriangle> projectedTriangles;
    for (int i = 0; i < n; i++) {
        projectedTriangles.push_back({ {dist(gen), dist(gen)}, {dist(gen), dist(gen)}, {dist(gen), dist(gen)} });
    }
    
    glGenBuffers(1, &projectedTrianglesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedTrianglesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                projectedTriangles.size() * sizeof(ProjectedTriangle),
                projectedTriangles.data(),
                GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, projectedTrianglesSSBO);
    
    projectedTriangleCountLoc = glGetUniformLocation(mainShaderProgram, "projectedTriangleCount");
    glUniform1i(projectedTriangleCountLoc, projectedTriangles.size());
}

void Renderer::render() {
    ZoneScoped;
    frame++;
    
    glUseProgram(mainShaderProgram);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedTrianglesSSBO);
    
    if (frame % 100000 == 0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        
        int n = 0;
        std::vector<ProjectedTriangle> projectedTriangles;
        for (int i = 0; i < n; i++) {
            projectedTriangles.push_back({ {dist(gen), dist(gen)}, {dist(gen), dist(gen)}, {dist(gen), dist(gen)} });
        }
        
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                        projectedTriangles.size() * sizeof(ProjectedTriangle),
                        projectedTriangles.data());
        glUniform1i(projectedTriangleCountLoc, projectedTriangles.size());
    }
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer::offload() {
    ZoneScoped;
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}
