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

void checkShaderCompilation(unsigned int shader, std::string errorMessage) {
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << errorMessage << "\n" << infoLog << std::endl;
    }
}

void checkShaderProgramLink(unsigned int shaderProgram, std::string errorMessage) {
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    
    if(!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << errorMessage << "\n" << infoLog << std::endl;
    }
}

void Renderer::load() {
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

    // triangle count shader
    fileLoader.loadFile("./shaders/count.glsl");
    std::string countShaderSource = fileLoader.getFileAsString();
    const char* countShaderChars = countShaderSource.c_str();
    
    countShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(countShader, 1, &countShaderChars, NULL);
    glCompileShader(countShader);
    
    checkShaderCompilation(countShader, "Error compiling count shader");
    
    countProgram = glCreateProgram();
    glAttachShader(countProgram, countShader);
    glLinkProgram(countProgram);
    
    checkShaderProgramLink(countProgram, "Error linking count shader program");

    // prefix sum compute shader
    fileLoader.loadFile("./shaders/prefix.glsl");
    std::string prefixShaderSource = fileLoader.getFileAsString();
    const char* prefixShaderChars = prefixShaderSource.c_str();
    
    prefixShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(prefixShader, 1, &prefixShaderChars, NULL);
    glCompileShader(prefixShader);
    
    checkShaderCompilation(prefixShader, "Error compiling prefix shader");
    
    prefixProgram = glCreateProgram();
    glAttachShader(prefixProgram, prefixShader);
    glLinkProgram(prefixProgram);
    
    checkShaderProgramLink(prefixProgram, "Error linking prefix shader program");

    // fill compute shader
    fileLoader.loadFile("./shaders/fill.glsl");
    std::string fillShaderSource = fileLoader.getFileAsString();
    const char* fillShaderChars = fillShaderSource.c_str();
    
    fillShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(fillShader, 1, &fillShaderChars, NULL);
    glCompileShader(fillShader);
    
    checkShaderCompilation(fillShader, "Error compiling fill shader");
    
    fillProgram = glCreateProgram();
    glAttachShader(fillProgram, fillShader);
    glLinkProgram(fillProgram);
    
    checkShaderProgramLink(fillProgram, "Error linking fill shader program");

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
    
    camera = Camera({-25, 0, 0}, {+1, 0, 0});
    
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
    glm::vec3 cubeOffset(-7.4f, 0.0f, 0.0f);
    std::vector<Triangle> cube = {
        // Front (z = 0.5)
        {rot * glm::vec4(-0.5, -0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5, -0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
        {rot * glm::vec4(-0.5, -0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, 0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
        // Back  (z = -0.5)
        {rot * glm::vec4( 0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
        {rot * glm::vec4( 0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
        // Right (x = 0.5)
        {rot * glm::vec4( 0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
        {rot * glm::vec4( 0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
        // Left  (x = -0.5)
        {rot * glm::vec4(-0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
        {rot * glm::vec4(-0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
        // Top   (y = 0.5)
        {rot * glm::vec4(-0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
        {rot * glm::vec4(-0.5,  0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5,  0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
        // Bottom (y = -0.5)
        {rot * glm::vec4(-0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
        {rot * glm::vec4(-0.5, -0.5, -0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4( 0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), rot * glm::vec4(-0.5, -0.5,  0.5, 1.0f) + glm::vec4(cubeOffset, 0.0f), 0},
    };
    std::vector<Material> materials = {
        {{1.0, 1.0, 1.0, 1.0}, -1},
    };
    
    int cubeCount = 10000;
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> posDist(-200.0f, 200.0f);
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> colour(0.0f, 1.0f);
    for (int i = 0; i < cubeCount; i++) {

        glm::vec3 offset(posDist(rng), posDist(rng), posDist(rng));

        glm::mat4 rot =
            glm::rotate(glm::mat4(1.0f), glm::radians(rotDist(rng)), glm::vec3(1,0,0)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(rotDist(rng)), glm::vec3(0,1,0)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(rotDist(rng)), glm::vec3(0,0,1));

        glm::mat4 model = glm::translate(glm::mat4(1.0f), offset) * rot;

        auto addTri = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, int d) {
            cube.push_back({
                model * glm::vec4(a, 1.0f),
                model * glm::vec4(b, 1.0f),
                model * glm::vec4(c, 1.0f),
                d
            });
        };

        addTri({-0.5,-0.5, 0.5},{ 0.5,-0.5, 0.5},{ 0.5, 0.5, 0.5},i+1);
        addTri({-0.5,-0.5, 0.5},{ 0.5, 0.5, 0.5},{-0.5, 0.5, 0.5},i+1);

        addTri({ 0.5,-0.5,-0.5},{-0.5,-0.5,-0.5},{-0.5, 0.5,-0.5},i+1);
        addTri({ 0.5,-0.5,-0.5},{-0.5, 0.5,-0.5},{ 0.5, 0.5,-0.5},i+1);

        addTri({ 0.5,-0.5,-0.5},{ 0.5,-0.5, 0.5},{ 0.5, 0.5, 0.5},i+1);
        addTri({ 0.5,-0.5,-0.5},{ 0.5, 0.5, 0.5},{ 0.5, 0.5,-0.5},i+1);

        addTri({-0.5,-0.5, 0.5},{-0.5,-0.5,-0.5},{-0.5, 0.5,-0.5},i+1);
        addTri({-0.5,-0.5, 0.5},{-0.5, 0.5,-0.5},{-0.5, 0.5, 0.5},i+1);

        addTri({-0.5, 0.5, 0.5},{ 0.5, 0.5, 0.5},{ 0.5, 0.5,-0.5},i+1);
        addTri({-0.5, 0.5, 0.5},{ 0.5, 0.5,-0.5},{-0.5, 0.5,-0.5},i+1);

        addTri({-0.5,-0.5,-0.5},{ 0.5,-0.5,-0.5},{ 0.5,-0.5, 0.5},i+1);
        addTri({-0.5,-0.5,-0.5},{ 0.5,-0.5, 0.5},{-0.5,-0.5, 0.5},i+1);
        
        materials.push_back({
            {colour(rng), colour(rng), colour(rng), 1.0},
            -1
        });
    }

    worldTriangleCount = cube.size();
    int materialCount = materials.size();

    tileColumns = 32;
    tileRows = 32;
    tileCount = tileRows * tileColumns;
    glGenBuffers(1, &tileTriangleCountSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, tileTriangleCountSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 tileCount * sizeof(unsigned int),
                 NULL,
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, tileTriangleCountSSBO);

    glGenBuffers(1, &tileOffsetsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, tileOffsetsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 (tileCount + 1) * sizeof(int),
                 NULL,
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, tileOffsetsSSBO);

    std::vector<int> zeroCounters(tileCount, 0);
    glGenBuffers(1, &tileCountersSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, tileCountersSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                tileCount * sizeof(int),
                zeroCounters.data(),
                GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, tileCountersSSBO);

    glGenBuffers(1, &tileTrianglesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, tileTrianglesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 tileCount * worldTriangleCount * 2 * sizeof(int),
                 NULL,
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, tileTrianglesSSBO);
    
    glGenBuffers(1, &materialsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 materialCount * sizeof(Material),
                 materials.data(),
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, materialsSSBO);
    
    glGenBuffers(1, &worldTrianglesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, worldTrianglesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 cube.size() * sizeof(Triangle),
                 cube.data(),
                 GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, worldTrianglesSSBO);
    
    glUseProgram(projectionProgram);
    worldTriangleCountLoc = glGetUniformLocation(projectionProgram, "triangleCount");
    glUniform1i(worldTriangleCountLoc, worldTriangleCount);
    
    glGenBuffers(1, &projectedTrianglesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedTrianglesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 worldTriangleCount * 2 * sizeof(ProjectedTriangle),
                 NULL,
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, projectedTrianglesSSBO);

    glUseProgram(countProgram);
    
    glUseProgram(projectionProgram);
    camPosLoc = glGetUniformLocation(projectionProgram, "cameraPosition");
    camFwdLoc = glGetUniformLocation(projectionProgram, "cameraForward");
    camUpLoc = glGetUniformLocation(projectionProgram, "cameraUp");
    
    glUseProgram(prefixProgram);
    prefixTileCountLoc = glGetUniformLocation(prefixProgram, "tileCount");

    glUseProgram(fillProgram);
    fillTriangleCountLoc = glGetUniformLocation(fillProgram, "projectedTriangleCount");
    fillTileRowsLoc = glGetUniformLocation(fillProgram, "tileRows");
    fillTileColumnsLoc = glGetUniformLocation(fillProgram, "tileColumns");
    
    glUseProgram(mainShaderProgram);
    projectedTriangleCountLoc = glGetUniformLocation(mainShaderProgram, "projectedTriangleCount");
    glUniform1i(projectedTriangleCountLoc, worldTriangleCount * 2);
    fragTileRowsLoc = glGetUniformLocation(mainShaderProgram, "tileRows");
    fragTileColumnsLoc = glGetUniformLocation(mainShaderProgram, "tileColumns");
    glUniform1i(fragTileRowsLoc, tileRows);
    glUniform1i(fragTileColumnsLoc, tileColumns);
}

void Renderer::render() {
    frame++;

    glUseProgram(projectionProgram);
    glUniform3fv(camPosLoc, 1, &camera.position[0]);
    glUniform3fv(camFwdLoc, 1, &camera.forward[0]);
    glUniform3fv(camUpLoc, 1, &camera.up[0]);
    glDispatchCompute((worldTriangleCount / 64) + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    int zero = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, tileTriangleCountSSBO);
    glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32I, 0, tileCount * sizeof(int), GL_RED_INTEGER, GL_INT, &zero);

    glUseProgram(countProgram);
    
    int countTriangleCountLoc = glGetUniformLocation(countProgram, "projectedTriangleCount");
    int tileRowsLoc = glGetUniformLocation(countProgram, "tileRows");
    int tileColumnsLoc = glGetUniformLocation(countProgram, "tileColumns");
    glUniform1i(countTriangleCountLoc, worldTriangleCount * 2);
    glUniform1i(tileRowsLoc, tileRows);
    glUniform1i(tileColumnsLoc, tileColumns);
    
    glDispatchCompute((worldTriangleCount * 2 / 64) + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(prefixProgram);
    glUniform1i(prefixTileCountLoc, tileCount);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, tileCountersSSBO);
    glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32I, 0, tileCount * sizeof(int), GL_RED_INTEGER, GL_INT, &zero);

    glUseProgram(fillProgram);
    glUniform1i(fillTriangleCountLoc, worldTriangleCount * 2);
    glUniform1i(fillTileRowsLoc, tileRows);
    glUniform1i(fillTileColumnsLoc, tileColumns);
    glDispatchCompute((worldTriangleCount * 2 / 64) + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(mainShaderProgram);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, projectedTrianglesSSBO);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer::offload() {
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}
