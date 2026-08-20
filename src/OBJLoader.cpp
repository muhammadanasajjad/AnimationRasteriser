#include <sstream>
#include <string>
#include <map>
#include <iostream>

#include <OBJLoader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

FileLoader OBJLoader::fileLoader;

static std::string getDirectory(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) return "./";
    return path.substr(0, lastSlash + 1);
}

static std::string resolveTexturePath(const std::string& mtlDir, const std::string& texPath) {
    if (texPath.empty()) return "";

    if (texPath[0] == '/') {
        size_t lastSlash = texPath.find_last_of('/');
        std::string filename = (lastSlash != std::string::npos) ? texPath.substr(lastSlash + 1) : texPath;
        std::string relative = mtlDir + filename;
        FILE* f = fopen(relative.c_str(), "rb");
        if (f) { fclose(f); return relative; }
        return texPath;
    }

    std::string full = mtlDir + texPath;
    FILE* f = fopen(full.c_str(), "rb");
    if (f) { fclose(f); return full; }
    return texPath;
}

static TextureData loadTexture(const std::string& path) {
    if (path.empty()) return {};

    int tw, th, tc;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &tw, &th, &tc, 4);
    if (!data) {
        std::cout << "Failed to load texture: " << path << " (" << stbi_failure_reason() << ")" << std::endl;
        return {};
    }

    TextureData tex;
    tex.width = tw;
    tex.height = th;
    tex.channels = 4;
    tex.pixels.assign(data, data + tw * th * 4);
    stbi_image_free(data);

    std::cout << "Loaded texture: " << path << " (" << tw << "x" << th << ")" << std::endl;
    return tex;
}

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

static void parseFaceEntry(const std::string& entry, unsigned int& vi, unsigned int& vti, unsigned int& vni) {
    vi = 0; vti = 0; vni = 0;

    size_t firstSlash = entry.find('/');
    size_t secondSlash = (firstSlash == std::string::npos) ? std::string::npos : entry.find('/', firstSlash + 1);

    vi = (unsigned int)std::stoi(entry.substr(0, firstSlash));

    if (firstSlash != std::string::npos && secondSlash != std::string::npos) {
        std::string vtStr = entry.substr(firstSlash + 1, secondSlash - firstSlash - 1);
        std::string vnStr = entry.substr(secondSlash + 1);
        if (!vtStr.empty()) vti = (unsigned int)std::stoi(vtStr);
        if (!vnStr.empty()) vni = (unsigned int)std::stoi(vnStr);
    } else if (firstSlash != std::string::npos) {
        std::string rest = entry.substr(firstSlash + 1);
        if (!rest.empty()) vti = (unsigned int)std::stoi(rest);
    }
}

Object OBJLoader::loadObject(std::string path, int materialIndex) {
    std::vector<glm::vec3> vertices = loadVertices(path);
    std::vector<glm::uvec3> faces = loadFaces(path);

    Object object;
    object.materialIndex = materialIndex;
    object.triangles.reserve(faces.size());
    for (glm::uvec3 face : faces) {
        Triangle tri;
        tri.p1 = glm::vec4(vertices[face.x - 1], 1.0f);
        tri.p2 = glm::vec4(vertices[face.y - 1], 1.0f);
        tri.p3 = glm::vec4(vertices[face.z - 1], 1.0f);
        tri.n1 = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        tri.n2 = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        tri.n3 = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        tri.materialIndex = materialIndex;
        object.triangles.push_back(tri);
    }

    return object;
}

Object OBJLoader::loadObject(std::string path, std::string mtlPath) {
    std::string mtlDir = getDirectory(mtlPath);

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;

    std::vector<Material> materials;
    std::vector<std::string> names;
    std::vector<std::string> texPaths;
    parseMTL(mtlPath, materials, names, texPaths);

    std::map<std::string, int> nameToIndex;
    for (size_t i = 0; i < names.size(); i++) {
        nameToIndex[names[i]] = (int)i;
    }

    Object object;
    object.materials = materials;
    object.textures.resize(materials.size());

    fileLoader.loadFile(path);
    std::string fileString = fileLoader.getFileAsString();
    std::stringstream fileStream(fileString);
    std::string line;

    int currentMaterial = 0;
    while (getline(fileStream, line)) {
        std::stringstream lineStream(line);
        std::string token;
        lineStream >> token;

        if (token == "v") {
            glm::vec3 v;
            lineStream >> v.x >> v.z >> v.y;
            vertices.push_back(v);
        } else if (token == "vt") {
            glm::vec2 vt;
            lineStream >> vt.x >> vt.y;
            texCoords.push_back(vt);
        } else if (token == "vn") {
            glm::vec3 vn;
            lineStream >> vn.x >> vn.z >> vn.y;
            normals.push_back(vn);
        } else if (token == "usemtl") {
            std::string name;
            lineStream >> name;
            std::map<std::string, int>::iterator it = nameToIndex.find(name);
            currentMaterial = (it != nameToIndex.end()) ? it->second : 0;
        } else if (token == "f") {
            std::vector<std::string> faceEntries;
            std::string entry;
            while (lineStream >> entry) {
                faceEntries.push_back(entry);
            }

            for (size_t i = 1; i + 1 < faceEntries.size(); i++) {
                unsigned int vi[3], vti[3], vni[3];
                parseFaceEntry(faceEntries[0], vi[0], vti[0], vni[0]);
                parseFaceEntry(faceEntries[i], vi[1], vti[1], vni[1]);
                parseFaceEntry(faceEntries[i + 1], vi[2], vti[2], vni[2]);

                Triangle tri;
                tri.p1 = glm::vec4(vertices[vi[0] - 1], 1.0f);
                tri.p2 = glm::vec4(vertices[vi[1] - 1], 1.0f);
                tri.p3 = glm::vec4(vertices[vi[2] - 1], 1.0f);

                tri.n1 = glm::vec4(vni[0] > 0 ? normals[vni[0] - 1] : glm::vec3(0.0f, 1.0f, 0.0f), 0.0f);
                tri.n2 = glm::vec4(vni[1] > 0 ? normals[vni[1] - 1] : glm::vec3(0.0f, 1.0f, 0.0f), 0.0f);
                tri.n3 = glm::vec4(vni[2] > 0 ? normals[vni[2] - 1] : glm::vec3(0.0f, 1.0f, 0.0f), 0.0f);

                tri.uv1 = vti[0] > 0 && vti[0] <= texCoords.size() ? texCoords[vti[0] - 1] : glm::vec2(0.0f);
                tri.uv2 = vti[1] > 0 && vti[1] <= texCoords.size() ? texCoords[vti[1] - 1] : glm::vec2(0.0f);
                tri.uv3 = vti[2] > 0 && vti[2] <= texCoords.size() ? texCoords[vti[2] - 1] : glm::vec2(0.0f);

                tri.materialIndex = currentMaterial;
                object.triangles.push_back(tri);
            }
        }
    }

    for (size_t i = 0; i < texPaths.size(); i++) {
        if (!texPaths[i].empty()) {
            object.textures[i] = loadTexture(resolveTexturePath(mtlDir, texPaths[i]));
            std::cout << "texture " << texPaths[i] << " loaded at " << i << std::endl;
        }
    }

    return object;
}

void OBJLoader::parseMTL(std::string mtlPath, std::vector<Material>& materials, std::vector<std::string>& names, std::vector<std::string>& texturePaths) {
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
            texturePaths.push_back("");
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
        } else if (token == "map_Kd") {
            if (!texturePaths.empty()) {
                std::string texPath;
                std::getline(lineStream, texPath);
                size_t start = texPath.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    texturePaths.back() = texPath.substr(start);
                }
            }
        }
    }
}
