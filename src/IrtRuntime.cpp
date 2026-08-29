#include <Irt.h>
#include <World.h>
#include <Animation.h>
#include <OBJLoader.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>

namespace irt {

namespace {

constexpr float CANVAS_DISTANCE = 10.0f;
constexpr float CANVAS_HALF_EXTENT = 8.0f;
constexpr float TEXT_CELL = 0.18f;
constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 6.2831853f;

bool fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

glm::vec3 canvasPoint(float u, float v) {
    return glm::vec3(CANVAS_DISTANCE, -u, -v);
}

glm::vec3 scenePoint(float x, float y, float z) {
    return glm::vec3(CANVAS_DISTANCE - z, -x, -y);
}

glm::vec3 localPoint(float u, float v) {
    return glm::vec3(0.0f, -u, -v);
}

const std::map<std::string, EasingFunc>& easingRegistry() {
    static const std::map<std::string, EasingFunc> registry = {
        {"linear", Easing::linear},
        {"easeInQuad", Easing::easeInQuad}, {"quadIn", Easing::easeInQuad},
        {"easeOutQuad", Easing::easeOutQuad}, {"quadOut", Easing::easeOutQuad},
        {"easeInOutQuad", Easing::easeInOutQuad}, {"quadInOut", Easing::easeInOutQuad},
        {"easeInCubic", Easing::easeInCubic}, {"cubicIn", Easing::easeInCubic},
        {"easeOutCubic", Easing::easeOutCubic}, {"cubicOut", Easing::easeOutCubic},
        {"easeInOutCubic", Easing::easeInOutCubic}, {"cubicInOut", Easing::easeInOutCubic},
        {"easeInSine", Easing::easeInSine}, {"sineIn", Easing::easeInSine},
        {"easeOutSine", Easing::easeOutSine}, {"sineOut", Easing::easeOutSine},
        {"easeInOutSine", Easing::easeInOutSine}, {"sineInOut", Easing::easeInOutSine},
        {"easeOutBack", Easing::easeOutBack}, {"backOut", Easing::easeOutBack},
        {"easeInBounce", Easing::easeInBounce}, {"bounceIn", Easing::easeInBounce},
        {"easeOutBounce", Easing::easeOutBounce}, {"bounceOut", Easing::easeOutBounce},
    };
    return registry;
}

EasingFunc resolveEasing(const TimelineCmd& command) {
    if (!command.hasEasing) return Easing::linear;
    const auto& registry = easingRegistry();
    auto it = registry.find(command.easing);
    if (it == registry.end()) {
        throw std::runtime_error("irt: unknown easing '" + command.easing + "' at line " +
                                 std::to_string(command.line) + " (available: linear, easeInQuad..., sineInOut, backOut, bounceOut)");
    }
    return it->second;
}

const std::map<std::string, std::array<const char*, 7>>& fontGlyphs() {
    static const std::map<std::string, std::array<const char*, 7>> glyphs = {
        {" ", {"00000", "00000", "00000", "00000", "00000", "00000", "00000"}},
        {"A", {"01110", "10001", "10001", "11111", "10001", "10001", "10001"}},
        {"B", {"11110", "10001", "10001", "11110", "10001", "10001", "11110"}},
        {"C", {"01110", "10001", "10000", "10000", "10000", "10001", "01110"}},
        {"D", {"11110", "10001", "10001", "10001", "10001", "10001", "11110"}},
        {"E", {"11111", "10000", "10000", "11110", "10000", "10000", "11111"}},
        {"F", {"11111", "10000", "10000", "11110", "10000", "10000", "10000"}},
        {"G", {"01110", "10001", "10000", "10111", "10001", "10001", "01111"}},
        {"H", {"10001", "10001", "10001", "11111", "10001", "10001", "10001"}},
        {"I", {"01110", "00100", "00100", "00100", "00100", "00100", "01110"}},
        {"J", {"00111", "00010", "00010", "00010", "00010", "10010", "01100"}},
        {"K", {"10001", "10010", "10100", "11000", "10100", "10010", "10001"}},
        {"L", {"10000", "10000", "10000", "10000", "10000", "10000", "11111"}},
        {"M", {"10001", "11011", "10101", "10101", "10001", "10001", "10001"}},
        {"N", {"10001", "11001", "10101", "10011", "10001", "10001", "10001"}},
        {"O", {"01110", "10001", "10001", "10001", "10001", "10001", "01110"}},
        {"P", {"11110", "10001", "10001", "11110", "10000", "10000", "10000"}},
        {"Q", {"01110", "10001", "10001", "10001", "10101", "10010", "01101"}},
        {"R", {"11110", "10001", "10001", "11110", "10100", "10010", "10001"}},
        {"S", {"01111", "10000", "10000", "01110", "00001", "00001", "11110"}},
        {"T", {"11111", "00100", "00100", "00100", "00100", "00100", "00100"}},
        {"U", {"10001", "10001", "10001", "10001", "10001", "10001", "01110"}},
        {"V", {"10001", "10001", "10001", "10001", "10001", "01010", "00100"}},
        {"W", {"10001", "10001", "10001", "10101", "10101", "11011", "10001"}},
        {"X", {"10001", "01010", "00100", "00100", "00100", "01010", "10001"}},
        {"Y", {"10001", "01010", "00100", "00100", "00100", "00100", "00100"}},
        {"Z", {"11111", "00001", "00010", "00100", "01000", "10000", "11111"}},
        {"0", {"01110", "10001", "10011", "10101", "11001", "10001", "01110"}},
        {"1", {"00100", "01100", "00100", "00100", "00100", "00100", "01110"}},
        {"2", {"01110", "10001", "00001", "00010", "00100", "01000", "11111"}},
        {"3", {"11111", "00010", "00100", "00010", "00001", "10001", "01110"}},
        {"4", {"00010", "00110", "01010", "10010", "11111", "00010", "00010"}},
        {"5", {"11111", "10000", "11110", "00001", "00001", "10001", "01110"}},
        {"6", {"00110", "01000", "10000", "11110", "10001", "10001", "01110"}},
        {"7", {"11111", "00001", "00010", "00100", "01000", "01000", "01000"}},
        {"8", {"01110", "10001", "10001", "01110", "10001", "10001", "01110"}},
        {"9", {"01110", "10001", "10001", "01111", "00001", "00010", "01100"}},
        {".", {"00000", "00000", "00000", "00000", "00000", "01100", "01100"}},
        {",", {"00000", "00000", "00000", "00000", "00110", "00110", "01000"}},
        {"!", {"00100", "00100", "00100", "00100", "00100", "00000", "00100"}},
        {"?", {"01110", "10001", "00001", "00110", "00100", "00000", "00100"}},
        {"-", {"00000", "00000", "00000", "01110", "00000", "00000", "00000"}},
        {"+", {"00000", "00100", "00100", "11111", "00100", "00100", "00000"}},
        {"=", {"00000", "00000", "11111", "00000", "11111", "00000", "00000"}},
        {":", {"00000", "01100", "01100", "00000", "01100", "01100", "00000"}},
        {"/", {"00001", "00010", "00010", "00100", "01000", "01000", "10000"}},
        {"(", {"00010", "00100", "01000", "01000", "01000", "00100", "00010"}},
        {")", {"01000", "00100", "00010", "00010", "00010", "00100", "01000"}},
        {"'", {"00100", "00100", "01000", "00000", "00000", "00000", "00000"}},
        {"\"", {"01010", "01010", "00000", "00000", "00000", "00000", "00000"}},
    };
    return glyphs;
}

struct Value {
    enum class Kind { Number, String, Boolean, Tuple };

    Kind kind = Kind::Number;
    double number = 0.0;
    bool boolean = false;
    std::string str;
    std::vector<double> tuple;
};

class Executor {
public:
    explicit Executor(World& world) : world(world) {}

    void execute(const Program& program, const std::string& dir) {
        baseDir = dir;
        registerProgram(program, dir);

        for (const Statement& statement : program.statements) {
            switch (statement.kind) {
                case Statement::Kind::Import:
                case Statement::Kind::Scene:
                    break;
                case Statement::Kind::Play:
                    playScene(statement.playTarget, statement.line);
                    break;
                case Statement::Kind::Wait:
                    if (!playedAnything) {
                        std::cout << "irt: warning: 'wait' with nothing playing (line "
                                  << statement.line << ")" << std::endl;
                    } else {
                        Timeline& timeline = world.getTimeline();
                        float pending = timeline.getDuration() - timeline.getCursor();
                        if (pending > 0.0f) timeline.wait(pending);
                    }
                    break;
            }
        }
    }

private:
    World& world;
    std::string baseDir = ".";
    std::map<std::string, const SceneDecl*> scenes;
    std::vector<std::unique_ptr<Program>> ownedPrograms;
    std::set<std::string> loadedFiles;
    bool playedAnything = false;

    void registerProgram(const Program& program, const std::string& dir) {
        for (const Statement& statement : program.statements) {
            if (statement.kind == Statement::Kind::Import) {
                loadFile(joinPath(dir, statement.importPath));
            } else if (statement.kind == Statement::Kind::Scene) {
                if (scenes.count(statement.scene.name)) {
                    throw std::runtime_error("irt: duplicate scene '" + statement.scene.name + "'");
                }
                scenes[statement.scene.name] = &statement.scene;
            }
        }
    }

    void loadFile(const std::string& path) {
        if (loadedFiles.count(path)) return;
        loadedFiles.insert(path);

        std::ifstream file(path);
        if (!file) throw std::runtime_error("irt: cannot open imported file '" + path + "'");

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        std::unique_ptr<Program> program(new Program(parse(tokenise(source), source)));
        registerProgram(*program, directoryOf(path));
        ownedPrograms.push_back(std::move(program));
    }

    void playScene(const std::string& name, int line) {
        auto it = scenes.find(name);
        if (it == scenes.end()) {
            std::string available;
            for (const auto& entry : scenes) {
                available += (available.empty() ? "" : ", ") + entry.first;
            }
            throw std::runtime_error("irt: unknown scene '" + name + "' at line " +
                                     std::to_string(line) + " (available: " + available + ")");
        }

        const SceneDecl& scene = *it->second;
        std::map<std::string, Object*> objectRefs;
        std::map<std::string, std::vector<glm::vec3>> pathRefs;

        for (const PathDecl& path : scene.paths) {
            std::vector<glm::vec3> points;
            for (const ExprPtr& point : path.points) {
                glm::vec3 v(0.0f);
                size_t count = std::min(point->items.size(), size_t(3));
                for (size_t i = 0; i < count; i++) {
                    v[i] = static_cast<float>(evaluateNumber(*point->items[i]));
                }
                points.push_back(scenePoint(v.x, v.y, v.z));
            }
            pathRefs[path.name] = points;
        }

        std::vector<const ObjectDecl*> graphDecls;
        for (const ObjectDecl& decl : scene.objects) {
            if (decl.kind == "graph") {
                graphDecls.push_back(&decl);
                continue;
            }
            if (decl.kind == "camera") {
                world.setSceneCamera(buildCamera(decl));
                continue;
            }
            Object* object = buildObject(decl, scene);
            objectRefs[decl.name] = object;
        }

        for (const ObjectDecl* decl : graphDecls) {
            Object* object = buildObject(*decl, scene);
            objectRefs[decl->name] = object;
        }

        Timeline& timeline = world.getTimeline();
        for (const TimelineCmd& command : scene.timeline) {
            emitCommand(timeline, command, objectRefs, pathRefs);
        }

        playedAnything = true;
        std::cout << "irt: playing scene '" << name << "' (" << scene.objects.size()
                  << " objects, timeline " << timeline.getDuration() << "s)" << std::endl;
    }

    const ObjectDecl& findAxesOrThrow(const SceneDecl& scene, const ObjectDecl& graph) const {
        for (const ObjectDecl& decl : scene.objects) {
            if (decl.name == graph.on) {
                if (decl.kind != "axes") {
                    throw std::runtime_error("irt: '" + graph.on + "' is a " + decl.kind +
                                             ", not axes, referenced by graph '" + graph.name + "'");
                }
                return decl;
            }
        }
        throw std::runtime_error("irt: graph '" + graph.name + "' references unknown axes '" + graph.on + "'");
    }

    struct Bounds {
        float minX = -10.0f;
        float maxX = 10.0f;
        float minY = -10.0f;
        float maxY = 10.0f;
    };

    Bounds axesBounds(const ObjectDecl& axesDecl) const {
        Bounds bounds;
        for (const Property& property : axesDecl.properties) {
            if (property.key != "x" && property.key != "y") continue;
            if (property.value->kind != ExprKind::Tuple || property.value->items.size() != 2) continue;
            double lo = evaluateNumber(*property.value->items[0]);
            double hi = evaluateNumber(*property.value->items[1]);
            if (property.key == "x") {
                bounds.minX = static_cast<float>(lo);
                bounds.maxX = static_cast<float>(hi);
            } else {
                bounds.minY = static_cast<float>(lo);
                bounds.maxY = static_cast<float>(hi);
            }
        }
        return bounds;
    }

    Object* buildObject(const ObjectDecl& decl, const SceneDecl& scene) {
        Object object;

        if (decl.kind == "text") buildText(object, decl);
        else if (decl.kind == "circle") buildCircle(object, decl);
        else if (decl.kind == "square") buildSquare(object, decl);
        else if (decl.kind == "axes") buildAxes(object, decl);
        else if (decl.kind == "graph") buildGraph(object, decl, scene);
        else if (decl.kind == "sphere") buildSphere(object, decl);
        else if (decl.kind == "cylinder") buildCylinder(object, decl);
        else if (decl.kind == "obj") buildObj(object, decl);

        object.visible = true;
        object.opacity = static_cast<float>(glm::clamp(propertyNumber(decl, "opacity", 1.0), 0.0, 1.0));
        object.layer = static_cast<int>(propertyNumber(decl, "layer", 0));

        if (decl.kind == "graph") {
            object.drawProgress = 0.0f;
        }

        object.name = decl.name;
        Object& stored = world.addObject(object);
        return &stored;
    }

    Camera buildCamera(const ObjectDecl& decl) {
        validateProperties(decl, {"position", "lookAt", "zoom"});

        Camera camera;
        camera.position = propertyPositionWorld(decl);

        glm::vec3 look = glm::vec3(0.0f);
        bool haveLook = false;
        for (const Property& property : decl.properties) {
            if (property.key != "lookAt") continue;
            const Expr& value = *property.value;
            if (value.kind == ExprKind::Tuple && !value.items.empty()) {
                float x = static_cast<float>(evaluateNumber(*value.items[0]));
                float y = value.items.size() > 1 ? static_cast<float>(evaluateNumber(*value.items[1])) : 0.0f;
                float z = value.items.size() > 2 ? static_cast<float>(evaluateNumber(*value.items[2])) : 0.0f;
                look = scenePoint(x, y, z);
                haveLook = true;
            }
        }
        if (!haveLook) look = scenePoint(0.0f, 0.0f, 0.0f);

        camera.zoom = static_cast<float>(propertyNumber(decl, "zoom", 1.0));
        camera.lookAt(look);
        return camera;
    }

    glm::vec3 propertyPositionWorld(const ObjectDecl& decl) {
        for (const Property& property : decl.properties) {
            if (property.key != "position") continue;
            const Expr& value = *property.value;
            if (value.kind == ExprKind::Tuple && !value.items.empty()) {
                float x = static_cast<float>(evaluateNumber(*value.items[0]));
                float y = value.items.size() > 1 ? static_cast<float>(evaluateNumber(*value.items[1])) : 0.0f;
                float z = value.items.size() > 2 ? static_cast<float>(evaluateNumber(*value.items[2])) : 0.0f;
                return scenePoint(x, y, z);
            }
        }
        return scenePoint(0.0f, 0.0f, 0.0f);
    }

    glm::vec4 propertyColour(const ObjectDecl& decl, const std::string& key, glm::vec4 fallback) {
        for (const Property& property : decl.properties) {
            if (property.key != key) continue;
            if (property.value->kind != ExprKind::Tuple || property.value->items.size() < 3) continue;
            glm::vec4 colour(fallback);
            colour.r = static_cast<float>(evaluateNumber(*property.value->items[0]));
            colour.g = static_cast<float>(evaluateNumber(*property.value->items[1]));
            colour.b = static_cast<float>(evaluateNumber(*property.value->items[2]));
            if (property.value->items.size() >= 4) {
                colour.a = static_cast<float>(evaluateNumber(*property.value->items[3]));
            }
            return colour;
        }
        return fallback;
    }

    glm::vec3 propertyTuple3(const ObjectDecl& decl, const std::string& key, glm::vec3 fallback) {
        for (const Property& property : decl.properties) {
            if (property.key != key) continue;
            if (property.value->kind != ExprKind::Tuple || property.value->items.empty()) continue;
            glm::vec3 value(fallback);
            size_t count = std::min(property.value->items.size(), size_t(3));
            for (size_t i = 0; i < count; i++) {
                value[i] = static_cast<float>(evaluateNumber(*property.value->items[i]));
            }
            return value;
        }
        return fallback;
    }

    glm::vec3 propertyPosition(const ObjectDecl& decl) {
        for (const Property& property : decl.properties) {
            if (property.key != "position") continue;
            const Expr& value = *property.value;

            if (value.kind == ExprKind::Identifier && value.text == "center") {
                return canvasPoint(0.0f, 0.0f);
            }
            if (value.kind == ExprKind::Identifier) {
                throw std::runtime_error("irt: unknown named position '" + value.text + "' on '" +
                                         decl.name + "' at line " + std::to_string(value.line) +
                                         " (available: center)");
            }
            if (value.kind == ExprKind::Tuple && !value.items.empty()) {
                float u = static_cast<float>(evaluateNumber(*value.items[0]));
                float v = value.items.size() > 1 ? static_cast<float>(evaluateNumber(*value.items[1])) : 0.0f;
                float w = value.items.size() > 2 ? static_cast<float>(evaluateNumber(*value.items[2])) : 0.0f;
                return scenePoint(u, v, w);
            }
        }
        return canvasPoint(0.0f, 0.0f);
    }

    double propertyNumber(const ObjectDecl& decl, const std::string& key, double fallback) {
        for (const Property& property : decl.properties) {
            if (property.key != key) continue;
            const Expr& value = *property.value;
            try {
                return evaluateNumber(value);
            } catch (const std::exception&) {
                return fallback;
            }
        }
        return fallback;
    }

    std::string propertyString(const ObjectDecl& decl, const std::string& key, const std::string& fallback) {
        for (const Property& property : decl.properties) {
            if (property.key != key) continue;
            if (property.value->kind == ExprKind::String) return property.value->text;
        }
        return fallback;
    }

    void validateProperties(const ObjectDecl& decl, const std::vector<std::string>& allowed) {
        for (const Property& property : decl.properties) {
            bool ok = false;
            for (const std::string& key : allowed) {
                if (property.key == key) {
                    ok = true;
                    break;
                }
            }
            if (!ok) {
                std::string valid;
                for (const std::string& key : allowed) valid += (valid.empty() ? "" : ", ") + key;
                throw std::runtime_error("irt: unknown property '" + property.key + "' on " + decl.kind +
                                         " '" + decl.name + "' at line " +
                                         std::to_string(property.value->line) + " (valid: " + valid + ")");
            }
        }
    }

    void addTriangle(Object& object, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
        addTriangleN(object, p1, p2, p3,
                     glm::vec3(1.0f, 0.0f, 0.0f),
                     glm::vec3(1.0f, 0.0f, 0.0f),
                     glm::vec3(1.0f, 0.0f, 0.0f));
    }

    void addTriangleN(Object& object, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
                      const glm::vec3& n1, const glm::vec3& n2, const glm::vec3& n3) {
        Triangle triangle{};
        triangle.p1 = glm::vec4(p1, 1.0f);
        triangle.p2 = glm::vec4(p2, 1.0f);
        triangle.p3 = glm::vec4(p3, 1.0f);
        triangle.n1 = glm::vec4(n1, 0.0f);
        triangle.n2 = glm::vec4(n2, 0.0f);
        triangle.n3 = glm::vec4(n3, 0.0f);
        triangle.materialIndex = 0;
        object.triangles.push_back(triangle);
    }

    void addQuad(Object& object, const glm::vec3& c1, const glm::vec3& c2,
                 const glm::vec3& c3, const glm::vec3& c4) {
        addTriangle(object, c1, c2, c3);
        addTriangle(object, c1, c3, c4);
    }

    void addRect(Object& object, float u0, float v0, float u1, float v1) {
        addQuad(object,
                localPoint(u0, v1),
                localPoint(u1, v1),
                localPoint(u1, v0),
                localPoint(u0, v0));
    }

    void buildSquare(Object& object, const ObjectDecl& decl) {
        validateProperties(decl, {"size", "position", "scale", "fill", "strokeWeight", "layer", "opacity"});

        float size = static_cast<float>(propertyNumber(decl, "size", 2.0));
        float half = size / 2.0f;

        object.transform.position = propertyPosition(decl);
        object.transform.scale = propertyTuple3(decl, "scale", glm::vec3(1.0f));

        Material material{};
        material.colour = propertyColour(decl, "fill", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        material.textureIndex = -1;
        object.materials.push_back(material);

        addRect(object, -half, -half, half, half);
    }

    void buildCircle(Object& object, const ObjectDecl& decl) {
        validateProperties(decl, {"radius", "position", "scale", "fill", "layer", "opacity"});

        float radius = static_cast<float>(propertyNumber(decl, "radius", 1.0));
        const int segments = 64;

        object.transform.position = propertyPosition(decl);
        object.transform.scale = propertyTuple3(decl, "scale", glm::vec3(1.0f));

        Material material{};
        material.colour = propertyColour(decl, "fill", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        material.textureIndex = -1;
        object.materials.push_back(material);

        glm::vec3 center = localPoint(0.0f, 0.0f);
        for (int i = 0; i < segments; i++) {
            float a0 = (6.2831853f * i) / segments;
            float a1 = (6.2831853f * (i + 1)) / segments;
            glm::vec3 p0 = localPoint(radius * std::cos(a0), radius * std::sin(a0));
            glm::vec3 p1 = localPoint(radius * std::cos(a1), radius * std::sin(a1));
            addTriangle(object, center, p0, p1);
        }
    }

    void buildText(Object& object, const ObjectDecl& decl) {
        validateProperties(decl, {"content", "position", "scale", "colour", "layer", "opacity"});

        std::string content = propertyString(decl, "content", "Hello");

        object.transform.position = propertyPosition(decl);
        object.transform.scale = propertyTuple3(decl, "scale", glm::vec3(1.0f));

        Material material{};
        material.colour = propertyColour(decl, "colour", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        material.textureIndex = -1;
        object.materials.push_back(material);

        float cell = TEXT_CELL;
        float totalWidth = (static_cast<float>(content.size()) * 6.0f - 1.0f) * cell;
        float penU = -totalWidth / 2.0f;

        for (char rawChar : content) {
            char c = static_cast<char>(std::toupper(static_cast<unsigned char>(rawChar)));
            const auto& glyphs = fontGlyphs();
            auto it = glyphs.find(std::string(1, c));
            if (it != glyphs.end() && c != ' ') {
                for (int row = 0; row < 7; row++) {
                    const char* bits = it->second[row];
                    for (int col = 0; col < 5; col++) {
                        if (bits[col] != '1') continue;

                        float u0 = penU + col * cell;
                        float u1 = u0 + cell;
                        float v1 = 3.5f * cell - row * cell;
                        float v0 = v1 - cell;

                        addRect(object, u0, v0, u1, v1);
                    }
                }
            }
            penU += 6.0f * cell;
        }
    }

    void buildAxes(Object& object, const ObjectDecl& decl) {
        validateProperties(decl, {"x", "y", "position", "layer"});

        Bounds bounds = axesBounds(decl);

        object.transform.position = propertyPosition(decl);

        Material material{};
        material.colour = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
        material.textureIndex = -1;
        object.materials.push_back(material);

        float xRange = bounds.maxX - bounds.minX;
        float yRange = bounds.maxY - bounds.minY;
        float fitScale = std::min(2.0f * CANVAS_HALF_EXTENT / xRange, 2.0f * CANVAS_HALF_EXTENT / yRange);

        float halfU = xRange * fitScale / 2.0f;
        float halfV = yRange * fitScale / 2.0f;
        float thickness = 0.06f;

        addRect(object, -halfU, -thickness / 2.0f, halfU, thickness / 2.0f);
        addRect(object, -thickness / 2.0f, -halfV, thickness / 2.0f, halfV);
    }

    void buildGraph(Object& object, const ObjectDecl& decl, const SceneDecl& scene) {
        validateProperties(decl, {"colour", "strokeWeight", "layer"});

        if (!decl.hasOn) {
            throw std::runtime_error("irt: graph '" + decl.name + "' has no 'on <axes>' target");
        }
        if (decl.equations.empty()) {
            throw std::runtime_error("irt: graph '" + decl.name + "' has no equations (e.g. 'y = x^2')");
        }

        const ObjectDecl& axesDecl = findAxesOrThrow(scene, decl);
        Bounds bounds = axesBounds(axesDecl);

        const Equation* xEquation = nullptr;
        const Equation* yEquation = nullptr;
        std::vector<std::string> params;
        for (const Equation& equation : decl.equations) {
            if (equation.target == "x") xEquation = &equation;
            else if (equation.target == "y") yEquation = &equation;
            for (const std::string& param : equation.params) {
                if (std::find(params.begin(), params.end(), param) == params.end()) {
                    params.push_back(param);
                }
            }
        }

        if (!yEquation && !xEquation) {
            throw std::runtime_error("irt: graph '" + decl.name + "' needs an 'x =' or 'y =' equation");
        }

        std::string paramName = "x";
        if (std::find(params.begin(), params.end(), "x") == params.end() && !params.empty()) {
            paramName = params[0];
        }

        float strokeWeight = static_cast<float>(propertyNumber(decl, "strokeWeight", 0.08));

        float axesOffsetU = 0.0f;
        float axesOffsetV = 0.0f;
        for (const Property& property : axesDecl.properties) {
            if (property.key == "position") {
                if (property.value->kind == ExprKind::Tuple && property.value->items.size() >= 2) {
                    axesOffsetU = static_cast<float>(evaluateNumber(*property.value->items[0]));
                    axesOffsetV = static_cast<float>(evaluateNumber(*property.value->items[1]));
                }
            }
        }

        object.transform.position = canvasPoint(axesOffsetU, axesOffsetV);

        Material material{};
        material.colour = propertyColour(decl, "colour", glm::vec4(0.2f, 0.6f, 1.0f, 1.0f));
        material.textureIndex = -1;
        object.materials.push_back(material);

        float xRange = bounds.maxX - bounds.minX;
        float yRange = bounds.maxY - bounds.minY;
        float fitScale = std::min(2.0f * CANVAS_HALF_EXTENT / xRange, 2.0f * CANVAS_HALF_EXTENT / yRange);
        float centerX = (bounds.minX + bounds.maxX) / 2.0f;
        float centerY = (bounds.minY + bounds.maxY) / 2.0f;

        const int samples = 240;
        bool havePrevious = false;
        float previousU = 0.0f;
        float previousV = 0.0f;

        for (int i = 0; i <= samples; i++) {
            double t = bounds.minX + (bounds.maxX - bounds.minX) * (double)i / samples;
            std::map<std::string, double> scope;
            scope[paramName] = t;

            double px = xEquation ? evaluateEquation(*xEquation, scope) : t;
            double py = yEquation ? evaluateEquation(*yEquation, scope) : 0.0;

            if (!std::isfinite(px) || !std::isfinite(py)) {
                havePrevious = false;
                continue;
            }

            float u = (static_cast<float>(px) - centerX) * fitScale;
            float v = (static_cast<float>(py) - centerY) * fitScale;

            if (havePrevious) {
                float du = u - previousU;
                float dv = v - previousV;
                float len = std::sqrt(du * du + dv * dv);
                if (len > 1e-6f) {
                    float nu = -dv / len * strokeWeight / 2.0f;
                    float nv = du / len * strokeWeight / 2.0f;
                    addQuad(object,
                            localPoint(previousU + nu, previousV + nv),
                            localPoint(u + nu, v + nv),
                            localPoint(u - nu, v - nv),
                            localPoint(previousU - nu, previousV - nv));
                }
            }

            havePrevious = true;
            previousU = u;
            previousV = v;
        }
    }

    void applyCommonTransform(Object& object, const ObjectDecl& decl) {
        object.transform.position = propertyPosition(decl);
        object.transform.scale = propertyTuple3(decl, "scale", glm::vec3(1.0f));
        object.transform.rotation = propertyTuple3(decl, "rotation", glm::vec3(0.0f));
    }

    void addPlainMaterial(Object& object, const ObjectDecl& decl) {
        Material material{};
        material.colour = propertyColour(decl, "fill", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        material.textureIndex = -1;
        object.materials.push_back(material);
    }

    void buildSphere(Object& object, const ObjectDecl& decl) {
        validateProperties(decl, {"radius", "segments", "position", "scale", "rotation", "fill", "layer", "opacity"});

        float radius = static_cast<float>(propertyNumber(decl, "radius", 1.0));
        int segments = static_cast<int>(propertyNumber(decl, "segments", 32));
        segments = std::max(4, std::min(segments, 256));
        int rings = std::max(segments / 2, 2);

        applyCommonTransform(object, decl);
        addPlainMaterial(object, decl);

        float safeRadius = std::max(radius, 1e-6f);
        auto spherePoint = [&](float phi, float theta) {
            return glm::vec3(radius * std::sin(phi) * std::cos(theta),
                             radius * std::sin(phi) * std::sin(theta),
                             radius * std::cos(phi));
        };
        auto sphereNormal = [&](const glm::vec3& p) { return p / safeRadius; };

        for (int s = 0; s < rings; s++) {
            float phi0 = kPi * s / rings;
            float phi1 = kPi * (s + 1) / rings;

            for (int i = 0; i < segments; i++) {
                float t0 = kTwoPi * i / segments;
                float t1 = kTwoPi * (i + 1) / segments;

                glm::vec3 p00 = spherePoint(phi0, t0);
                glm::vec3 p01 = spherePoint(phi0, t1);
                glm::vec3 p10 = spherePoint(phi1, t0);
                glm::vec3 p11 = spherePoint(phi1, t1);

                if (s == 0) {
                    addTriangleN(object, glm::vec3(0.0f, 0.0f, radius), p10, p11,
                                 glm::vec3(0.0f, 0.0f, 1.0f), sphereNormal(p10), sphereNormal(p11));
                } else if (s == rings - 1) {
                    addTriangleN(object, p00, p01, glm::vec3(0.0f, 0.0f, -radius),
                                 sphereNormal(p00), sphereNormal(p01), glm::vec3(0.0f, 0.0f, -1.0f));
                } else {
                    addTriangleN(object, p00, p11, p10,
                                 sphereNormal(p00), sphereNormal(p11), sphereNormal(p10));
                    addTriangleN(object, p00, p01, p11,
                                 sphereNormal(p00), sphereNormal(p01), sphereNormal(p11));
                }
            }
        }
    }

    void buildCylinder(Object& object, const ObjectDecl& decl) {
        validateProperties(decl, {"radius", "height", "segments", "position", "scale", "rotation", "fill", "layer", "opacity"});

        float radius = static_cast<float>(propertyNumber(decl, "radius", 1.0));
        float height = static_cast<float>(propertyNumber(decl, "height", 2.0));
        int segments = static_cast<int>(propertyNumber(decl, "segments", 32));
        segments = std::max(3, std::min(segments, 256));

        applyCommonTransform(object, decl);
        addPlainMaterial(object, decl);

        float half = height / 2.0f;
        auto ringPoint = [&](float theta, float z) {
            return glm::vec3(radius * std::cos(theta), radius * std::sin(theta), z);
        };

        for (int i = 0; i < segments; i++) {
            float t0 = kTwoPi * i / segments;
            float t1 = kTwoPi * (i + 1) / segments;

            glm::vec3 n0(std::cos(t0), std::sin(t0), 0.0f);
            glm::vec3 n1(std::cos(t1), std::sin(t1), 0.0f);

            glm::vec3 b0 = ringPoint(t0, -half);
            glm::vec3 b1 = ringPoint(t1, -half);
            glm::vec3 c0 = ringPoint(t0, half);
            glm::vec3 c1 = ringPoint(t1, half);

            addTriangleN(object, b0, b1, c1, n0, n1, n1);
            addTriangleN(object, b0, c1, c0, n0, n1, n0);

            addTriangleN(object, glm::vec3(0.0f, 0.0f, half), c0, c1,
                         glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            addTriangleN(object, glm::vec3(0.0f, 0.0f, -half), b1, b0,
                         glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        }
    }

    void buildObj(Object& object, const ObjectDecl& decl) {
        validateProperties(decl, {"path", "mtlPath", "position", "scale", "rotation", "fill", "layer", "opacity"});

        std::string path = propertyString(decl, "path", "");
        if (path.empty()) {
            throw std::runtime_error("irt: obj '" + decl.name + "' needs a 'path: \"model.obj\"' property");
        }
        path = joinPath(baseDir, path);
        if (!fileExists(path)) {
            throw std::runtime_error("irt: obj '" + decl.name + "' cannot open '" + path + "'");
        }

        std::string mtlPath = propertyString(decl, "mtlPath", "");
        if (!mtlPath.empty()) {
            mtlPath = joinPath(baseDir, mtlPath);
            if (!fileExists(mtlPath)) {
                throw std::runtime_error("irt: obj '" + decl.name + "' cannot open mtl '" + mtlPath + "'");
            }
        } else {
            size_t dot = path.find_last_of('.');
            std::string stem = (dot == std::string::npos) ? path : path.substr(0, dot);
            std::string sidecar = stem + ".mtl";
            if (fileExists(sidecar)) mtlPath = sidecar;
        }

        if (!mtlPath.empty()) {
            object = OBJLoader::loadObject(path, mtlPath);
        } else {
            object = OBJLoader::loadObject(path);
            addPlainMaterial(object, decl);
        }

        applyCommonTransform(object, decl);
    }

    void emitBarrier(Timeline& timeline) {
        float pending = timeline.getDuration() - timeline.getCursor();
        if (pending > 0.0f) timeline.wait(pending);
    }

    void emitCommand(Timeline& timeline, const TimelineCmd& command,
                     const std::map<std::string, Object*>& objectRefs,
                     const std::map<std::string, std::vector<glm::vec3>>& pathRefs) {
        Object* target = nullptr;
        Camera* camTarget = nullptr;
        bool cameraCommand = command.target == "camera";
        if (command.kind != TimelineCmd::Kind::Wait && command.kind != TimelineCmd::Kind::Serial) {
            if (!cameraCommand) {
                auto it = objectRefs.find(command.target);
                if (it == objectRefs.end()) {
                    throw std::runtime_error("irt: timeline references unknown object '" + command.target +
                                             "' at line " + std::to_string(command.line));
                }
                target = it->second;
            } else {
                camTarget = world.activeCamera();
            }
        }

        EasingFunc easing = resolveEasing(command);

        switch (command.kind) {
            case TimelineCmd::Kind::FadeIn:
                timeline.fadeIn(*target, command.duration, easing);
                break;
            case TimelineCmd::Kind::FadeOut:
                timeline.fadeOut(*target, command.duration, easing);
                break;
            case TimelineCmd::Kind::Wait:
                if (command.bareWait) {
                    emitBarrier(timeline);
                } else {
                    timeline.wait(command.duration);
                }
                break;
            case TimelineCmd::Kind::Remove:
                timeline.hide(*target);
                break;
            case TimelineCmd::Kind::Rotate: {
                glm::vec3 rotation(0.0f);
                size_t count = std::min(command.rotation->items.size(), size_t(3));
                for (size_t i = 0; i < count; i++) {
                    rotation[i] = static_cast<float>(evaluateNumber(*command.rotation->items[i]));
                }
                timeline.rotate(*target, rotation, command.duration, easing);
                break;
            }
            case TimelineCmd::Kind::Draw:
                timeline.draw(*target, command.duration, easing);
                break;
            case TimelineCmd::Kind::Move: {
                if (command.destinationIsPath) {
                    auto pathIt = pathRefs.find(command.destination->text);
                    if (pathIt == pathRefs.end()) {
                        throw std::runtime_error("irt: move references unknown path '" +
                                                 command.destination->text + "' at line " +
                                                 std::to_string(command.line));
                    }
                    if (cameraCommand) {
                        timeline.moveCamera(*camTarget, pathIt->second, command.duration, easing, command.oriented);
                    } else {
                        timeline.move(*target, pathIt->second, command.duration, easing, command.oriented);
                    }
                } else {
                    glm::vec3 destination(0.0f);
                    size_t count = std::min(command.destination->items.size(), size_t(3));
                    for (size_t i = 0; i < count; i++) {
                        destination[i] = static_cast<float>(evaluateNumber(*command.destination->items[i]));
                    }
                    glm::vec3 world = scenePoint(destination.x, destination.y, destination.z);
                    if (cameraCommand) {
                        timeline.moveCamera(*camTarget, world, command.duration, easing);
                    } else {
                        timeline.move(*target, world, command.duration, easing);
                    }
                }
                break;
            }
            case TimelineCmd::Kind::Look: {
                glm::vec3 destination(0.0f);
                size_t count = std::min(command.destination->items.size(), size_t(3));
                for (size_t i = 0; i < count; i++) {
                    destination[i] = static_cast<float>(evaluateNumber(*command.destination->items[i]));
                }
                timeline.lookCamera(*camTarget, scenePoint(destination.x, destination.y, destination.z),
                                    command.duration, easing);
                break;
            }
            case TimelineCmd::Kind::Serial:
                for (const TimelineCmd& child : command.children) {
                    emitCommand(timeline, child, objectRefs, pathRefs);
                    emitBarrier(timeline);
                }
                break;
            }
    }

    double evaluateNumber(const Expr& expr, const std::map<std::string, double>& scope = {}) const {
        switch (expr.kind) {
            case ExprKind::Number:
                return expr.number;
            case ExprKind::Boolean:
                return expr.boolean ? 1.0 : 0.0;
            case ExprKind::Identifier: {
                auto scoped = scope.find(expr.text);
                if (scoped != scope.end()) return scoped->second;

                if (expr.text == "lowerBound") return -10.0;
                if (expr.text == "upperBound") return 10.0;
                if (expr.text == "r" || expr.text == "g" || expr.text == "b" || expr.text == "a") return 1.0;
                if (expr.text == "pitch" || expr.text == "yaw" || expr.text == "roll") return 0.0;
                if (expr.text == "x" || expr.text == "y" || expr.text == "z") return 0.0;

                throw std::runtime_error("irt: unknown identifier '" + expr.text + "' at line " +
                                         std::to_string(expr.line) +
                                         " (defaults exist for x/y/z, r/g/b/a, lowerBound/upperBound, pitch/yaw/roll)");
            }
            case ExprKind::UnaryMinus:
                return -evaluateNumber(*expr.left, scope);
            case ExprKind::Binary: {
                double l = evaluateNumber(*expr.left, scope);
                double r = evaluateNumber(*expr.right, scope);
                if (expr.text == "+") return l + r;
                if (expr.text == "-") return l - r;
                if (expr.text == "*") return l * r;
                if (expr.text == "/") return l / r;
                if (expr.text == "^") return std::pow(l, r);
                if (expr.text == "==") return l == r ? 1.0 : 0.0;
                if (expr.text == "!=") return l != r ? 1.0 : 0.0;
                if (expr.text == ">") return l > r ? 1.0 : 0.0;
                if (expr.text == "<") return l < r ? 1.0 : 0.0;
                if (expr.text == ">=") return l >= r ? 1.0 : 0.0;
                if (expr.text == "<=") return l <= r ? 1.0 : 0.0;
                throw std::runtime_error("irt: unknown operator '" + expr.text + "' at line " +
                                         std::to_string(expr.line));
            }
            default:
                throw std::runtime_error("irt: expression cannot be evaluated as a number at line " +
                                         std::to_string(expr.line));
        }
    }

    double evaluateEquation(const Equation& equation, const std::map<std::string, double>& scope) const {
        return evaluateNumber(*equation.expr, scope);
    }

    static std::string joinPath(const std::string& dir, const std::string& relative) {
        if (dir.empty() || (!relative.empty() && relative.front() == '/')) return relative;
        std::vector<std::string> parts;
        std::string combined = dir + "/" + relative;
        std::stringstream stream(combined);
        std::string part;
        while (std::getline(stream, part, '/')) {
            if (part.empty() || part == ".") continue;
            if (part == "..") {
                if (!parts.empty()) parts.pop_back();
            } else {
                parts.push_back(part);
            }
        }
        std::string joined;
        for (const std::string& p : parts) joined += (joined.empty() ? "" : "/") + p;
        return combined.front() == '/' ? "/" + joined : joined;
    }

    static std::string directoryOf(const std::string& path) {
        size_t slash = path.find_last_of('/');
        if (slash == std::string::npos) return ".";
        if (slash == 0) return "/";
        return path.substr(0, slash);
    }
};

}

void playSceneFile(World& world, const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("irt: cannot open scene file '" + path + "'");

    world.useCanvasCamera();

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    Program program = parse(tokenise(source), source);

    std::string dir = ".";
    size_t slash = path.find_last_of('/');
    if (slash != std::string::npos) dir = path.substr(0, slash);

    Executor executor(world);
    executor.execute(program, dir);
}

}
