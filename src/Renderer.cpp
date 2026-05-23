#include <glad/glad.h>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <Renderer.h>
#include <FileLoader.h>
#include <RendererStructs.h>

void Renderer::load() {
    FileLoader fileLoader = FileLoader();
    
    // vertex shader load
    fileLoader.loadFile("./shaders/screenVertex.vert");
    std::string vertexShaderSource = fileLoader.getFileAsString();
    const char* vertexShaderChars = vertexShaderSource.c_str();
    
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    
    glShaderSource(vertexShader, 1, &vertexShaderChars, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "Error vertex shader compilation failed \n" << infoLog << std::endl;
        return;
    }
    
    // fragment shader load
    fileLoader.loadFile("./shaders/screenFragment.frag");
    std::string fragmentShaderSource = fileLoader.getFileAsString();
    const char* fragmentShaderChars = fragmentShaderSource.c_str();
    
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(fragmentShader, 1, &fragmentShaderChars, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Error fragment shader compilation failed \n" << infoLog << std::endl;
        return;
    }
    
    // Link vertex and fragment shader
    mainShaderProgram = glCreateProgram();
    glAttachShader(mainShaderProgram, vertexShader);
    glAttachShader(mainShaderProgram, fragmentShader);
    glLinkProgram(mainShaderProgram);
    
    glGetProgramiv(mainShaderProgram, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(mainShaderProgram, 512, NULL, infoLog);
        std::cout << "Linking main shader program failed \n" << infoLog << std::endl;
        return;
    }
    
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
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    int n = 500;
    std::vector<ProjectedTriangle> projectedTriangles;
    for (int i = 0; i < n; i++) {
        projectedTriangles.push_back({ {dist(gen), dist(gen)} });
    }
    
    glGenBuffers(1, &projectedTrianglesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedTrianglesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                projectedTriangles.size() * sizeof(ProjectedTriangle),
                projectedTriangles.data(),
                GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, projectedTrianglesSSBO);
}

void Renderer::render() {
    frame++;
    
    glUseProgram(mainShaderProgram);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedTrianglesSSBO);
    
    if (frame % 100 == 0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        int n = 500;
        std::vector<ProjectedTriangle> projectedTriangles;
        for (int i = 0; i < n; i++) {
            projectedTriangles.push_back({ {dist(gen), dist(gen)} });
        }
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                        projectedTriangles.size() * sizeof(ProjectedTriangle),
                        projectedTriangles.data());
    }
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer::offload() {
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}
