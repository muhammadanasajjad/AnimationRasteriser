#include <Irt.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <sstream>

namespace irt {

std::string getLineText(const std::string& source, int lineNumber) {
    std::istringstream stream(source);
    std::string line;
    int current = 1;
    while (std::getline(stream, line)) {
        if (current == lineNumber) return line;
        current++;
    }
    return "";
}

namespace {

bool isIdentifierStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool isIdentifierChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

struct PunctRule {
    const char* text;
};

const PunctRule punctRules[] = {
    {"=="}, {"!="}, {">="}, {"<="}, {"&&"}, {"||"},
    {"{"}, {"}"}, {"("}, {")"}, {":"}, {";"}, {","},
    {"^"}, {"="}, {">"}, {"<"}, {"+"}, {"-"}, {"*"}, {"/"},
};

}

std::vector<Token> tokenise(const std::string& source) {
    std::vector<Token> tokens;
    size_t pos = 0;
    int line = 1;

    auto fail = [&](const std::string& message) {
        throw ParseError(message + " at line " + std::to_string(line) + ": " + getLineText(source, line));
    };

    while (pos < source.size()) {
        char c = source[pos];

        if (c == ' ' || c == '\t' || c == '\r' || c == ';') {
            pos++;
            continue;
        }

        if (c == '\n') {
            line++;
            pos++;
            continue;
        }

        if (c == '/' && pos + 1 < source.size() && source[pos + 1] == '/') {
            while (pos < source.size() && source[pos] != '\n') pos++;
            continue;
        }

        if (c == '"') {
            size_t end = pos + 1;
            while (end < source.size() && source[end] != '"' && source[end] != '\n') end++;
            if (end >= source.size() || source[end] != '"') fail("unterminated string literal");
            tokens.push_back({TokenType::String, source.substr(pos + 1, end - pos - 1), 0.0, line});
            pos = end + 1;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            size_t end = pos;
            while (end < source.size() && std::isdigit(static_cast<unsigned char>(source[end]))) end++;
            if (end < source.size() && source[end] == '.' &&
                end + 1 < source.size() && std::isdigit(static_cast<unsigned char>(source[end + 1]))) {
                end++;
                while (end < source.size() && std::isdigit(static_cast<unsigned char>(source[end]))) end++;
            }
            tokens.push_back({TokenType::Number, source.substr(pos, end - pos), std::stod(source.substr(pos, end - pos)), line});
            pos = end;
            continue;
        }

        if (isIdentifierStart(c)) {
            size_t end = pos;
            while (end < source.size() && isIdentifierChar(source[end])) end++;
            tokens.push_back({TokenType::Identifier, source.substr(pos, end - pos), 0.0, line});
            pos = end;
            continue;
        }

        bool matchedPunct = false;
        for (const PunctRule& rule : punctRules) {
            size_t len = std::strlen(rule.text);
            if (source.compare(pos, len, rule.text) == 0) {
                tokens.push_back({TokenType::Punct, rule.text, 0.0, line});
                pos += len;
                matchedPunct = true;
                break;
            }
        }
        if (matchedPunct) continue;

        fail(std::string("unexpected character '") + c + "'");
    }

    return tokens;
}

namespace {

class Parser {
public:
    Parser(const std::vector<Token>& tokens, const std::string& source)
        : tokens(tokens), source(source) {}

    Program run() {
        Program program;

        while (!atEnd()) {
            const Token& token = peek();
            Statement statement;

            if (token.type == TokenType::Identifier && token.text == "import") {
                statement = parseImport();
            } else if (token.type == TokenType::Identifier && token.text == "scene") {
                statement.kind = Statement::Kind::Scene;
                statement.scene = parseScene();
                statement.line = statement.scene.line;
            } else if (token.type == TokenType::Identifier && token.text == "play") {
                advance();
                statement.kind = Statement::Kind::Play;
                statement.playTarget = consumeIdentifier("scene name after 'play'");
                statement.line = token.line;
            } else if (token.type == TokenType::Identifier && token.text == "wait") {
                advance();
                statement.kind = Statement::Kind::Wait;
                statement.line = token.line;
            } else {
                error(token, "expected import/scene/play/wait");
            }

            program.statements.push_back(std::move(statement));
        }

        return program;
    }

private:
    const std::vector<Token>& tokens;
    const std::string& source;
    size_t pos = 0;

    struct BindingPower {
        double left;
        double right;
    };

    bool atEnd() const {
        return pos >= tokens.size();
    }

    const Token& peek(size_t offset = 0) const {
        static Token endToken{TokenType::End, "", 0.0, 0};
        if (pos + offset >= tokens.size()) return endToken;
        return tokens[pos + offset];
    }

    const Token& advance() {
        if (atEnd()) throw ParseError("unexpected end of file");
        return tokens[pos++];
    }

    void error(const Token& token, const std::string& message) const {
        throw ParseError(message + " at line " + std::to_string(token.line) + ": " + getLineText(source, token.line));
    }

    bool matchPunct(const std::string& text) {
        if (peek().type == TokenType::Punct && peek().text == text) {
            advance();
            return true;
        }
        return false;
    }

    void expectPunct(const std::string& text) {
        const Token& token = peek();
        if (token.type != TokenType::Punct || token.text != text) {
            error(token, "expected '" + text + "', got '" + token.text + "'");
        }
        advance();
    }

    std::string consumeIdentifier(const std::string& what) {
        const Token& token = peek();
        if (token.type != TokenType::Identifier) {
            error(token, "expected " + what + ", got '" + token.text + "'");
        }
        advance();
        return token.text;
    }

    Statement parseImport() {
        const Token& start = advance();

        Statement statement;
        statement.kind = Statement::Kind::Import;
        statement.line = start.line;
        statement.importBinding = consumeIdentifier("import name after 'import'");

        const Token& fromToken = peek();
        if (fromToken.type != TokenType::Identifier || fromToken.text != "from") {
            error(fromToken, "expected 'from' in import statement");
        }
        advance();

        const Token& pathToken = peek();
        if (pathToken.type != TokenType::String) {
            error(pathToken, "expected file path string after 'from'");
        }
        advance();
        statement.importPath = pathToken.text;

        if (statement.importPath.empty()) {
            error(pathToken, "import path cannot be empty");
        }

        return statement;
    }

    SceneDecl parseScene() {
        const Token& start = advance();

        SceneDecl scene;
        scene.line = start.line;
        scene.name = consumeIdentifier("scene name after 'scene'");

        expectPunct("{");

        while (!(peek().type == TokenType::Punct && peek().text == "}")) {
            const Token& token = peek();

            if (token.type == TokenType::End) {
                error(token, "unexpected end of file inside scene '" + scene.name + "'");
            }

            if (token.type == TokenType::Identifier && token.text == "timeline") {
                if (!scene.timeline.empty()) {
                    error(token, "duplicate timeline block in scene '" + scene.name + "'");
                }
                scene.timeline = parseTimeline(scene.name);
            } else if (token.type == TokenType::Identifier && token.text == "path") {
                parsePath(scene);
            } else {
                scene.objects.push_back(parseObject(scene.name));
            }
        }

        expectPunct("}");
        return scene;
    }

    void parsePath(SceneDecl& scene) {
        const Token& start = advance();

        PathDecl path;
        path.line = start.line;
        path.name = consumeIdentifier("path name after 'path'");

        for (const PathDecl& existing : scene.paths) {
            if (existing.name == path.name) {
                error(start, "duplicate path '" + path.name + "' in scene '" + scene.name + "'");
            }
        }

        expectPunct("{");

        while (!(peek().type == TokenType::Punct && peek().text == "}")) {
            if (peek().type == TokenType::End) {
                error(peek(), "unexpected end of file inside path '" + path.name + "'");
            }
            ExprPtr point = parseExpression();
            if (point->kind != ExprKind::Tuple ||
                point->items.size() < 2 || point->items.size() > 3) {
                error(peek(), "path points must be tuples like '(x, y)' or '(x, y, z)'");
            }
            path.points.push_back(std::move(point));

            if (peek().type == TokenType::Punct && peek().text == ",") {
                advance();
            }
        }

        expectPunct("}");

        if (path.points.size() < 3 || path.points.size() % 2 == 0) {
            error(start, "path '" + path.name + "' needs an odd number of points "
                         "(on, off, on, off, on, ...)");
        }

        scene.paths.push_back(std::move(path));
    }

    ObjectDecl parseObject(const std::string& sceneName) {
        const Token& start = peek();
        ObjectDecl object;
        object.line = start.line;

        object.kind = consumeIdentifier("object kind");
        if (object.kind != "text" && object.kind != "circle" && object.kind != "square" &&
            object.kind != "axes" && object.kind != "graph" && object.kind != "sphere" &&
            object.kind != "cylinder" && object.kind != "obj" && object.kind != "camera") {
            error(start, "unknown object kind '" + object.kind + "' in scene '" + sceneName +
                             "' (expected text/circle/square/axes/graph/sphere/cylinder/obj/camera)");
        }

        object.name = consumeIdentifier("object name");

        if (peek().type == TokenType::Identifier && peek().text == "on") {
            advance();
            object.hasOn = true;
            object.on = consumeIdentifier("axes name after 'on'");
        }

        expectPunct("{");

        while (!(peek().type == TokenType::Punct && peek().text == "}")) {
            if (peek().type == TokenType::End) {
                error(peek(), "unexpected end of file inside object '" + object.name + "'");
            }

            if (peek().type == TokenType::Identifier && peek(1).type == TokenType::Punct && peek(1).text == "=") {
                Equation equation;
                equation.line = peek().line;
                equation.target = consumeIdentifier("equation target");
                expectPunct("=");
                equation.expr = parseExpression();
                collectFreeIdentifiers(*equation.expr, equation.params);
                object.equations.push_back(std::move(equation));
            } else {
                Property property;
                property.key = consumeIdentifier("property name");
                expectPunct(":");
                property.value = parseExpression();
                object.properties.push_back(std::move(property));
            }
        }

        expectPunct("}");
        return object;
    }

    std::vector<TimelineCmd> parseTimeline(const std::string& sceneName) {
        std::vector<TimelineCmd> commands;

        advance();
        expectPunct("{");

        while (!(peek().type == TokenType::Punct && peek().text == "}")) {
            commands.push_back(parseTimelineCommand(sceneName));
        }

        expectPunct("}");
        return commands;
    }

    TimelineCmd parseTimelineCommand(const std::string& sceneName) {
        const Token& start = peek();

        if (start.type != TokenType::Identifier) {
            error(start, "expected timeline command in scene '" + sceneName + "'");
        }

        TimelineCmd command;
        command.line = start.line;

        if (start.text == "fade") {
            advance();
            command.target = consumeIdentifier("target object after 'fade'");

            const Token& direction = peek();
            if (direction.type == TokenType::Identifier && direction.text == "in") {
                command.kind = TimelineCmd::Kind::FadeIn;
            } else if (direction.type == TokenType::Identifier && direction.text == "out") {
                command.kind = TimelineCmd::Kind::FadeOut;
            } else {
                error(direction, "expected 'in' or 'out' after fade target");
            }
            advance();

            const Token& fadeOverToken = peek();
            if (fadeOverToken.type != TokenType::Identifier || fadeOverToken.text != "over") {
                error(fadeOverToken, "expected 'over <duration>' after fade direction (e.g. 'fade x in over 1s')");
            }
            advance();

            command.duration = parseDuration();
            parseEasingClause(command);
        } else if (start.text == "wait") {
            advance();
            command.kind = TimelineCmd::Kind::Wait;
            if (peek().type == TokenType::Number) {
                command.duration = parseDuration();
            } else {
                command.bareWait = true;
            }
        } else if (start.text == "serial") {
            advance();
            command.kind = TimelineCmd::Kind::Serial;
            expectPunct("{");
            while (!(peek().type == TokenType::Punct && peek().text == "}")) {
                if (peek().type == TokenType::End) {
                    error(peek(), "unexpected end of file inside serial block in scene '" + sceneName + "'");
                }
                command.children.push_back(parseTimelineCommand(sceneName));
            }
            expectPunct("}");
        } else if (start.text == "remove") {
            advance();
            command.kind = TimelineCmd::Kind::Remove;
            command.target = consumeIdentifier("target object after 'remove'");
        } else if (start.text == "rotate") {
            advance();
            command.kind = TimelineCmd::Kind::Rotate;
            command.target = consumeIdentifier("target object after 'rotate'");

            const Token& toToken = peek();
            if (toToken.type != TokenType::Identifier || toToken.text != "to") {
                error(toToken, "expected 'to' after rotate target");
            }
            advance();

            command.rotation = parseExpression();
            if (command.rotation->kind != ExprKind::Tuple) {
                error(toToken, "rotate requires a tuple like '(pitch, yaw, roll)'");
            }

            const Token& overToken = peek();
            if (overToken.type != TokenType::Identifier || overToken.text != "over") {
                error(overToken, "expected 'over <duration>' after rotate tuple");
            }
            advance();

            command.duration = parseDuration();
            parseEasingClause(command);
        } else if (start.text == "move") {
            advance();
            command.kind = TimelineCmd::Kind::Move;
            command.target = consumeIdentifier("target object after 'move'");

            const Token& toToken = peek();
            if (toToken.type != TokenType::Identifier || toToken.text != "to") {
                error(toToken, "expected 'to' after move target");
            }
            advance();

            ExprPtr destination = parseExpression();
            if (destination->kind == ExprKind::Tuple) {
                if (destination->items.size() < 2 || destination->items.size() > 3) {
                    error(toToken, "move requires a tuple like '(x, y)' or '(x, y, z)'");
                }
            } else if (destination->kind == ExprKind::Identifier) {
                command.destinationIsPath = true;
            } else {
                error(toToken, "move requires a tuple '(x, y, z)' or a path name after 'to'");
            }
            command.destination = std::move(destination);

            if (peek().type == TokenType::Identifier && peek().text == "oriented") {
                if (!command.destinationIsPath) {
                    error(peek(), "'oriented' requires a path destination");
                }
                advance();
                command.oriented = true;
            }

            const Token& overToken = peek();
            if (overToken.type != TokenType::Identifier || overToken.text != "over") {
                error(overToken, "expected 'over <duration>' after move destination");
            }
            advance();

            command.duration = parseDuration();
            parseEasingClause(command);
        } else if (start.text == "look") {
            advance();
            command.kind = TimelineCmd::Kind::Look;

            const Token& cameraToken = peek();
            if (cameraToken.type != TokenType::Identifier || cameraToken.text != "camera") {
                error(cameraToken, "expected 'camera' after 'look'");
            }
            advance();
            command.target = "camera";

            const Token& atToken = peek();
            if (atToken.type != TokenType::Identifier || atToken.text != "at") {
                error(atToken, "expected 'at' after 'look camera'");
            }
            advance();

            ExprPtr destination = parseExpression();
            if (destination->kind != ExprKind::Tuple ||
                destination->items.size() < 2 || destination->items.size() > 3) {
                error(atToken, "look camera requires a tuple like '(x, y)' or '(x, y, z)'");
            }
            command.destination = std::move(destination);

            const Token& overToken = peek();
            if (overToken.type != TokenType::Identifier || overToken.text != "over") {
                error(overToken, "expected 'over <duration>' after look camera at tuple");
            }
            advance();

            command.duration = parseDuration();
            parseEasingClause(command);
        } else if (start.text == "draw") {
            advance();
            command.kind = TimelineCmd::Kind::Draw;
            command.target = consumeIdentifier("target object after 'draw'");

            const Token& overToken = peek();
            if (overToken.type != TokenType::Identifier || overToken.text != "over") {
                error(overToken, "expected 'over <duration>' after draw target (e.g. 'draw x over 1s')");
            }
            advance();

            command.duration = parseDuration();
            parseEasingClause(command);
        } else {
            error(start, "unknown timeline command '" + start.text +
                             "' (expected fade/wait/remove/rotate/draw/move/look/serial)");
        }

        return command;
    }

    float parseDuration() {
        const Token& token = peek();
        if (token.type != TokenType::Number) {
            error(token, "expected duration like '1s' or '500ms'");
        }
        advance();

        float seconds = static_cast<float>(token.number);

        const Token& unit = peek();
        if (unit.type == TokenType::Identifier && (unit.text == "s" || unit.text == "ms")) {
            advance();
            if (unit.text == "ms") seconds /= 1000.0f;
        } else if (unit.type == TokenType::Identifier) {
            error(unit, "unknown duration unit '" + unit.text + "' (use 's' or 'ms')");
        }

        return seconds;
    }

    void parseEasingClause(TimelineCmd& command) {
        const Token& token = peek();
        if (token.type == TokenType::Identifier && token.text == "easing") {
            advance();
            command.hasEasing = true;
            command.easing = consumeIdentifier("easing name after 'easing'");
        }
    }

    ExprPtr parseExpression(double minBinding = 0.0) {
        ExprPtr left = parseFactor();

        while (true) {
            const Token& token = peek();
            if (token.type != TokenType::Punct) break;

            BindingPower power = binaryBindingPower(token.text);
            if (power.left < 0.0 || power.left <= minBinding) break;

            advance();
            ExprPtr right = parseExpression(power.right);

            ExprPtr node(new Expr());
            node->kind = ExprKind::Binary;
            node->text = token.text;
            node->line = token.line;
            node->left = std::move(left);
            node->right = std::move(right);
            left = std::move(node);
        }

        return left;
    }

    BindingPower binaryBindingPower(const std::string& op) const {
        if (op == "||") return {1.1, 1.0};
        if (op == "&&") return {2.1, 2.0};
        if (op == "==" || op == "!=" || op == ">" || op == "<" || op == ">=" || op == "<=") return {3.1, 3.0};
        if (op == "+" || op == "-") return {4.1, 4.0};
        if (op == "*" || op == "/") return {5.1, 5.0};
        if (op == "^") return {6.1, 6.0};
        return {-1.0, -1.0};
    }

    ExprPtr parseFactor() {
        const Token& token = peek();

        if (token.type == TokenType::Number) {
            advance();
            ExprPtr node(new Expr());
            node->kind = ExprKind::Number;
            node->number = token.number;
            node->line = token.line;
            return node;
        }

        if (token.type == TokenType::String) {
            advance();
            ExprPtr node(new Expr());
            node->kind = ExprKind::String;
            node->text = token.text;
            node->line = token.line;
            return node;
        }

        if (token.type == TokenType::Identifier && (token.text == "true" || token.text == "false")) {
            advance();
            ExprPtr node(new Expr());
            node->kind = ExprKind::Boolean;
            node->boolean = token.text == "true";
            node->line = token.line;
            return node;
        }

        if (token.type == TokenType::Punct && token.text == "-") {
            advance();
            ExprPtr operand = parseFactor();
            ExprPtr node(new Expr());
            node->kind = ExprKind::UnaryMinus;
            node->line = token.line;
            node->left = std::move(operand);
            return node;
        }

        if (token.type == TokenType::Identifier) {
            advance();
            ExprPtr node(new Expr());
            node->kind = ExprKind::Identifier;
            node->text = token.text;
            node->line = token.line;
            return node;
        }

        if (token.type == TokenType::Punct && token.text == "(") {
            int line = token.line;
            advance();

            ExprPtr first = parseExpression();

            if (peek().type == TokenType::Punct && peek().text == ",") {
                ExprPtr node(new Expr());
                node->kind = ExprKind::Tuple;
                node->line = line;
                node->items.push_back(std::move(first));

                while (matchPunct(",")) {
                    node->items.push_back(parseExpression());
                }

                expectPunct(")");
                return node;
            }

            expectPunct(")");
            first->line = line;
            return first;
        }

        error(token, "unexpected token '" + token.text + "' in expression");
        return nullptr;
    }

    void collectFreeIdentifiers(const Expr& expr, std::vector<std::string>& out) const {
        switch (expr.kind) {
            case ExprKind::Identifier:
                if (std::find(out.begin(), out.end(), expr.text) == out.end()) {
                    out.push_back(expr.text);
                }
                break;
            case ExprKind::UnaryMinus:
                collectFreeIdentifiers(*expr.left, out);
                break;
            case ExprKind::Binary:
                collectFreeIdentifiers(*expr.left, out);
                collectFreeIdentifiers(*expr.right, out);
                break;
            case ExprKind::Tuple:
                for (const ExprPtr& item : expr.items) {
                    collectFreeIdentifiers(*item, out);
                }
                break;
            default:
                break;
        }
    }
};

}

Program parse(const std::vector<Token>& tokens, const std::string& source) {
    Parser parser(tokens, source);
    return parser.run();
}

}
