#include <fstream>
#include <iostream>

#include <chess.hpp>
#include <PGNLoader.h>

class PGNVisitor : public chess::pgn::Visitor {
public:
    PGNGame game;
    bool skipGame = false;

    void startPgn() override {
        skipGame = false;
        game = PGNGame();
    }

    void header(std::string_view key, std::string_view value) override {
        if (key == "Event") game.event = value;
        else if (key == "White") game.white = value;
        else if (key == "Black") game.black = value;
        else if (key == "Result") game.result = value;
        else if (key == "FEN") game.fen = value;
    }

    void startMoves() override {}

    void move(std::string_view move, std::string_view comment) override {
        if (skipGame || move.empty()) return;
        if (move[0] == '{' || move[0] == ':' || move[0] == ']' || move[0] == ';') return;
        game.sanMoves.emplace_back(move);
    }

    void endPgn() override {}
};

PGNGame PGNLoader::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open PGN: " << path << std::endl;
        return {};
    }

    chess::pgn::StreamParser<> parser(file);
    PGNVisitor visitor;
    auto error = parser.readGames(visitor);

    if (error.hasError()) {
        std::cerr << "PGN parse error: " << error.message() << std::endl;
    }

    PGNGame& game = visitor.game;

    std::cout << "Loaded PGN: " << game.white << " vs " << game.black
              << " (" << game.sanMoves.size() << " half-moves)" << std::endl;

    return game;
}
