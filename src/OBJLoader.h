#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <FileLoader.h>
#include <RendererStructs.h>
#include <World.h>

class OBJLoader {
    public:
        OBJLoader() = delete;

        static std::vector<glm::vec3> loadVertices(std::string path);
        static std::vector<glm::uvec3> loadFaces(std::string path);
        static Object loadObject(std::string path, int materialIndex = 0);

    private:
        static FileLoader fileLoader;
};
