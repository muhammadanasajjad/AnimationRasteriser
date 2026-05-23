#include <glad/glad.h>
#include <Renderer.h>
#include <FileLoader.h>
#include <iostream>
#include <fstream>
#include <string>

void Renderer::load() {
    FileLoader fileLoader = FileLoader();
    
    // vertex shader load
    fileLoader.loadFile("./shaders/triangleVertex.vert");
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
    fileLoader.loadFile("./shaders/triangleFragment.frag");
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
    
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::render() {
    std::cout << "frame rendering should be done now";
    std::cout << " " << frame << std::endl;
    frame++;
    
    glUseProgram(mainShaderProgram);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::offload() {
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}
