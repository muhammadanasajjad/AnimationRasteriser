#pragma once

#include <Camera.h>

class Renderer {
    public:
        void load();
        void loadShaders();
        void initBuffers();
        void initScene();
        void render();
        void offload();
    private:
        int frame = 0;
        
        Camera camera;
        
        // SHADERS
        unsigned int vertexShader;
        unsigned int fragmentShader;
        unsigned int mainShaderProgram;
        unsigned int projectionShader;
        unsigned int projectionProgram;
        unsigned int VAO, VBO;
        unsigned int projectedTrianglesSSBO;
        unsigned int projectedTriangleCountLoc;
};
