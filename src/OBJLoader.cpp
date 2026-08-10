#include <sstream>
#include <string>
#include <OBJLoader.h>

FileLoader OBJLoader::fileLoader;

std::vector<glm::vec3> OBJLoader::loadVertices(std::string path) {
    fileLoader.loadFile(path);
    std::string fileString = fileLoader.getFileAsString();

    std::vector<glm::vec3> vertices;

    std::stringstream fileStream(fileString);
    std::string line;

    while (getline(fileStream, line)) {
        std::stringstream lineStream(line);
        std::string token;
        lineStream >> token;

        if (token == "v") {
            glm::vec3 vertex = {vertex.x, vertex.z * - 1, vertex.y};
            vertices.push_back(vertex);
        }
    }

    return vertices;
}

std::vector<glm::uvec3> OBJLoader::loadFaces(std::string path) {
    fileLoader.loadFile(path);
    std::string fileString = fileLoader.getFileAsString();

    std::vector<glm::uvec3> faces;

    std::stringstream fileStream(fileString);
    std::string line;

    while (getline(fileStream, line)) {
        std::stringstream lineStream(line);
        std::string token;
        lineStream >> token;

        if (token == "f") {
            std::vector<unsigned int> faceVertices;
            std::string entry;
            while (lineStream >> entry) {
                faceVertices.push_back((unsigned int)std::stoi(entry.substr(0, entry.find('/'))));
            }

            for (int i = 1; i + 1 < (int)faceVertices.size(); i++) {
                faces.push_back({faceVertices[0], faceVertices[i], faceVertices[i + 1]});
            }
        }
    }

    return faces;
}

std::vector<Triangle> OBJLoader::loadTriangles(std::string path) {
    std::vector<glm::vec3> vertices = loadVertices(path);
    std::vector<glm::uvec3> faces = loadFaces(path);

    std::vector<Triangle> triangles;
    triangles.reserve(faces.size());
    for (glm::uvec3 face : faces) {
        triangles.push_back({
            glm::vec4(vertices[face.x - 1], 1.0f),
            glm::vec4(vertices[face.y - 1], 1.0f),
            glm::vec4(vertices[face.z - 1], 1.0f),
            0
        });
    }

    return triangles;
}
