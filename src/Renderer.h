#pragma once

#include <Camera.h>

class Renderer {
    public:
        void load();
        void render();
        void offload();
        
        Camera camera;
    private:
        int frame = 0;
        
        unsigned int VAO, VBO;

        unsigned int tileColumns;
        unsigned int tileRows;
        unsigned int tileCount;
        
        // SSBOs
        unsigned int projectedTrianglesSSBO; // binding 0
        unsigned int worldTrianglesSSBO;     // binding 1
        unsigned int tileTriangleCountSSBO;  // binding 2
        unsigned int tileOffsetsSSBO;        // binding 3
        unsigned int tileCountersSSBO;       // binding 4
        unsigned int tileTrianglesSSBO;      // binding 5
        unsigned int materialsSSBO;          // binding 6
        
        // projection compute
        unsigned int projectionProgram;
        unsigned int projectionShader;
        unsigned int worldTriangleCount;
        unsigned int worldTriangleCountLoc;
        unsigned int camPosLoc, camFwdLoc, camUpLoc;
        
        // count compute
        unsigned int countProgram;
        unsigned int countShader;
        
        // prefix compute
        unsigned int prefixProgram;
        unsigned int prefixShader;
        unsigned int prefixTileCountLoc;
        
        // fill compute
        unsigned int fillProgram;
        unsigned int fillShader;
        unsigned int fillTriangleCountLoc;
        unsigned int fillTileRowsLoc;
        unsigned int fillTileColumnsLoc;
        
        // main render
        unsigned int mainShaderProgram;
        unsigned int vertexShader;
        unsigned int fragmentShader;
        unsigned int projectedTriangleCountLoc;
        unsigned int fragTileRowsLoc;
        unsigned int fragTileColumnsLoc;
};
