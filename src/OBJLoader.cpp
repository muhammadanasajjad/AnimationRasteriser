#include <sstream>
#include <string>
#include <map>

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
            glm::vec3 vertex;
            lineStream >> vertex.x >> vertex.z >> vertex.y;
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

Object OBJLoader::loadObject(std::string path, int materialIndex) {
    std::vector<glm::vec3> vertices = loadVertices(path);
    std::vector<glm::uvec3> faces = loadFaces(path);

    Object object;
    object.materialIndex = materialIndex;
    object.triangles.reserve(faces.size());
    for (glm::uvec3 face : faces) {
        object.triangles.push_back({
            glm::vec4(vertices[face.x - 1], 1.0f),
            glm::vec4(vertices[face.y - 1], 1.0f),
            glm::vec4(vertices[face.z - 1], 1.0f),
            materialIndex
        });
    }

    return object;
}

Object OBJLoader::loadObject(std::string path, std::string mtlPath) {
    std::vector<glm::vec3> vertices = loadVertices(path);

    std::vector<Material> materials;
    std::vector<std::string> names;
    parseMTL(mtlPath, materials, names);

    std::map<std::string, int> nameToIndex;
    for (size_t i = 0; i < names.size(); i++) {
        nameToIndex[names[i]] = (int)i;
    }

    Object object;
    object.materials = materials;

    fileLoader.loadFile(path);
    std::string fileString = fileLoader.getFileAsString();
    std::stringstream fileStream(fileString);
    std::string line;

    int currentMaterial = 0;
    while (getline(fileStream, line)) {
        std::stringstream lineStream(line);
        std::string token;
        lineStream >> token;

        if (token == "usemtl") {
            std::string name;
            lineStream >> name;
            std::map<std::string, int>::iterator it = nameToIndex.find(name);
            currentMaterial = (it != nameToIndex.end()) ? it->second : 0;
        } else if (token == "f") {
            std::vector<unsigned int> faceVertices;
            std::string entry;
            while (lineStream >> entry) {
                faceVertices.push_back((unsigned int)std::stoi(entry.substr(0, entry.find('/'))));
            }

            for (int i = 1; i + 1 < (int)faceVertices.size(); i++) {
                object.triangles.push_back({
                    glm::vec4(vertices[faceVertices[0] - 1], 1.0f),
                    glm::vec4(vertices[faceVertices[i] - 1], 1.0f),
                    glm::vec4(vertices[faceVertices[i + 1] - 1], 1.0f),
                    currentMaterial
                });
            }
        }
    }

    return object;
}

void OBJLoader::parseMTL(std::string mtlPath, std::vector<Material>& materials, std::vector<std::string>& names) {
    fileLoader.loadFile(mtlPath);
    std::string fileString = fileLoader.getFileAsString();

    std::stringstream fileStream(fileString);
    std::string line;

    while (getline(fileStream, line)) {
        std::stringstream lineStream(line);
        std::string token;
        lineStream >> token;

        if (token == "newmtl") {
            std::string name;
            lineStream >> name;
            names.push_back(name);
            materials.push_back({{1.0, 1.0, 1.0, 1.0}, -1});
        } else if (token == "Kd") {
            if (!materials.empty()) {
                glm::vec3 colour;
                lineStream >> colour.x >> colour.y >> colour.z;
                materials.back().colour = glm::vec4(colour, materials.back().colour.a);
            }
        } else if (token == "d") {
            if (!materials.empty()) {
                float alpha;
                lineStream >> alpha;
                materials.back().colour.a = alpha;
            }
        }
    }
}
