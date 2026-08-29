#pragma once

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class World;

namespace irt {

enum class TokenType {
    End,
    Identifier,
    Number,
    String,
    Punct
};

struct Token {
    TokenType type = TokenType::End;
    std::string text;
    double number = 0.0;
    int line = 0;
};

std::vector<Token> tokenise(const std::string& source);

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

enum class ExprKind {
    Number,
    String,
    Boolean,
    Identifier,
    Tuple,
    UnaryMinus,
    Binary
};

struct Expr {
    ExprKind kind = ExprKind::Number;

    double number = 0.0;
    bool boolean = false;
    std::string text;

    std::vector<ExprPtr> items;
    ExprPtr left;
    ExprPtr right;

    int line = 0;
};

struct Property {
    std::string key;
    ExprPtr value;
};

struct Equation {
    std::string target;
    std::vector<std::string> params;
    ExprPtr expr;
    int line = 0;
};

struct ObjectDecl {
    std::string kind;
    std::string name;
    std::string on;
    bool hasOn = false;
    std::vector<Property> properties;
    std::vector<Equation> equations;
    int line = 0;
};

struct TimelineCmd {
    enum class Kind {
        FadeIn,
        FadeOut,
        Wait,
        Remove,
        Rotate,
        Draw,
        Move,
        Look,
        Serial
    };

    Kind kind = Kind::Wait;
    std::string target;
    float duration = 0.0f;
    bool bareWait = false;
    std::string easing;
    bool hasEasing = false;
    ExprPtr rotation;
    ExprPtr destination;
    bool destinationIsPath = false;
    bool oriented = false;
    std::vector<TimelineCmd> children;
    int line = 0;
};

struct PathDecl {
    std::string name;
    std::vector<ExprPtr> points;
    int line = 0;
};

struct SceneDecl {
    std::string name;
    std::vector<ObjectDecl> objects;
    std::vector<PathDecl> paths;
    std::vector<TimelineCmd> timeline;
    int line = 0;
};

struct Statement {
    enum class Kind {
        Import,
        Scene,
        Play,
        Wait
    };

    Kind kind = Kind::Wait;

    std::string importBinding;
    std::string importPath;
    SceneDecl scene;
    std::string playTarget;

    int line = 0;
};

struct Program {
    std::vector<Statement> statements;
};

Program parse(const std::vector<Token>& tokens, const std::string& source);

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message) : std::runtime_error(message) {}
};

std::string getLineText(const std::string& source, int lineNumber);

void playSceneFile(World& world, const std::string& path);

}
