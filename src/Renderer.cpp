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

static_assert(sizeof(Triangle) == 128, "Triangle must be 128 bytes for std430 alignment");
static_assert(sizeof(ProjectedTriangle) == 112, "ProjectedTriangle must be 112 bytes for std430 alignment");

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

void Renderer::load(const std::vector<Triangle>& triangles, const std::vector<Material>& materials, const std::vector<TextureData>& textures) {
    std::vector<Material> mats = materials;
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

    camera = Camera({0, -35, -25}, {0, 1, 0.9f});

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

    worldTriangleCount = triangles.size();
    std::cout << worldTriangleCount << std::endl;
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
                 triangles.size() * sizeof(Triangle),
                 triangles.data(),
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, worldTrianglesSSBO);

    glUseProgram(projectionProgram);
    worldTriangleCountLoc = glGetUniformLocation(projectionProgram, "triangleCount");
    glUniform1i(worldTriangleCountLoc, worldTriangleCount);

    aspectRatioLoc = glGetUniformLocation(projectionProgram, "aspectRatio");
    glUniform1f(aspectRatioLoc, 1.0f);

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

    // lighting
    lightDirLoc = glGetUniformLocation(mainShaderProgram, "lightDir");
    glm::vec3 ld = glm::normalize(lightDirection);
    glUniform3fv(lightDirLoc, 1, &ld[0]);

    // textures
    textureCountLoc = glGetUniformLocation(mainShaderProgram, "textureCount");
    int texCount = 0;

    for (size_t i = 0; i < textures.size(); i++) {
        const TextureData& texData = textures[i];
        if (texData.pixels.empty()) {
            mats[i].textureIndex = -1;
            continue;
        }

        unsigned int tex;
        glGenTextures(1, &tex);

        glActiveTexture(GL_TEXTURE0 + texCount);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texData.width, texData.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData.pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        texturesGPU.push_back(tex);

        std::string texName = "textures[" + std::to_string(texCount) + "]";
        glUniform1i(glGetUniformLocation(mainShaderProgram, texName.c_str()), texCount);
        mats[i].textureIndex = texCount;
        texCount++;

        std::cout << "Material " << i << " -> texture unit " << texCount - 1 << std::endl;
    }

    glUniform1i(textureCountLoc, texCount);

    std::cout << "\n=== Material -> Texture Mapping ===" << std::endl;
    for (size_t i = 0; i < mats.size(); i++) {
        std::cout << "Material[" << i << "] textureIndex=" << mats[i].textureIndex
                  << " colour=(" << mats[i].colour.r << "," << mats[i].colour.g << "," << mats[i].colour.b << ")" << std::endl;
    }
    std::cout << "textureCount=" << texCount << std::endl << std::endl;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 materialCount * sizeof(Material),
                 mats.data(),
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, materialsSSBO);

    materialsCPU = mats;
}

void Renderer::updateTriangles(const std::vector<Triangle>& triangles) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, worldTrianglesSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, triangles.size() * sizeof(Triangle), triangles.data());

    worldTriangleCount = triangles.size();
    glUseProgram(projectionProgram);
    glUniform1i(worldTriangleCountLoc, worldTriangleCount);
}

void Renderer::setMaterialAlpha(int index, float alpha) {
    if (index < 0 || index >= (int)materialsCPU.size()) return;

    materialsCPU[index].colour.a = alpha;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialsSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                    index * sizeof(Material),
                    sizeof(Material),
                    &materialsCPU[index]);
}

void Renderer::setAspectRatio(float aspect) {
    glUseProgram(projectionProgram);
    glUniform1f(aspectRatioLoc, aspect);
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
    for (unsigned int tex : texturesGPU) {
        glDeleteTextures(1, &tex);
    }
}
