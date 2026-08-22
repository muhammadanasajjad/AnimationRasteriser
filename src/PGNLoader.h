#pragma once

#include <string>
#include <vector>

struct PGNGame {
    std::string event, white, black, result, fen;
    std::vector<std::string> sanMoves;
};

class PGNLoader {
public:
    static PGNGame load(const std::string& path);
};
