#include <iostream>
#include <cmath>

#include <glm/glm.hpp>

#include <chess.hpp>
#include <chessSimpleAnimation.h>
#include <World.h>
#include <OBJLoader.h>
#include <PGNLoader.h>
#include <Animation.h>

static constexpr float ROOK_POS = 20.0f;
static constexpr float Z_PIECE = -5.0f;

static float fileToX(int file) {
    float spacing = (2.0f * ROOK_POS) / 7.0f;
    return -ROOK_POS + file * spacing;
}

static float rankToY(int rank) {
    float spacing = (2.0f * ROOK_POS) / 7.0f;
    return -ROOK_POS + rank * spacing;
}

static Object& placePiece(World& world, const Object& tmpl, float x, float y) {
    Object copy = tmpl;
    copy.transform.rotation = glm::vec3(180.0f, 0.0f, 0.0f);
    copy.transform.position = glm::vec3(x, y, Z_PIECE);
    return world.addObject(copy);
}

void chessSimpleAnimation(World& world) {
    Object white_pawn = OBJLoader::loadObject("assets/chess/pieces/white_pawn.obj", "assets/chess/pieces/white_pawn.mtl");
    Object boardObj = OBJLoader::loadObject("assets/chess/board.obj", "assets/chess/board.mtl");
    boardObj.transform.scale = glm::vec3(4.2f);
    world.addObject(boardObj);

    float eFileX = fileToX(4);
    float rank2Y = rankToY(1);
    Object& pawn = placePiece(world, white_pawn, eFileX, rank2Y);

    world.addGlobalLight(glm::vec3(0.5f, 1.0f, -1.0f));
    world.getTimeline().move(pawn, glm::vec3(eFileX, rankToY(3), Z_PIECE), 1.0f, Easing::easeInOutSine);
}

void chessPGNAnimation(World& world, const std::string& pgnPath) {
    Object whiteTemplates[6];
    whiteTemplates[0] = OBJLoader::loadObject("assets/chess/pieces/white_pawn.obj",   "assets/chess/pieces/white_pawn.mtl");
    whiteTemplates[1] = OBJLoader::loadObject("assets/chess/pieces/white_knight.obj", "assets/chess/pieces/white_knight.mtl");
    whiteTemplates[2] = OBJLoader::loadObject("assets/chess/pieces/white_bishop.obj", "assets/chess/pieces/white_bishop.mtl");
    whiteTemplates[3] = OBJLoader::loadObject("assets/chess/pieces/white_rook.obj",   "assets/chess/pieces/white_rook.mtl");
    whiteTemplates[4] = OBJLoader::loadObject("assets/chess/pieces/white_queen.obj",  "assets/chess/pieces/white_queen.mtl");
    whiteTemplates[5] = OBJLoader::loadObject("assets/chess/pieces/white_king.obj",   "assets/chess/pieces/white_king.mtl");

    Object blackTemplates[6];
    blackTemplates[0] = OBJLoader::loadObject("assets/chess/pieces/black_pawn.obj",   "assets/chess/pieces/black_pawn.mtl");
    blackTemplates[1] = OBJLoader::loadObject("assets/chess/pieces/black_knight.obj", "assets/chess/pieces/black_knight.mtl");
    blackTemplates[2] = OBJLoader::loadObject("assets/chess/pieces/black_bishop.obj", "assets/chess/pieces/black_bishop.mtl");
    blackTemplates[3] = OBJLoader::loadObject("assets/chess/pieces/black_rook.obj",   "assets/chess/pieces/black_rook.mtl");
    blackTemplates[4] = OBJLoader::loadObject("assets/chess/pieces/black_queen.obj",  "assets/chess/pieces/black_queen.mtl");
    blackTemplates[5] = OBJLoader::loadObject("assets/chess/pieces/black_king.obj",   "assets/chess/pieces/black_king.mtl");

    Object* grid[8][8] = {};

    auto pieceTemplateIndex = [](chess::PieceType pt) -> int {
        switch (static_cast<int>(pt)) {
            case static_cast<int>(chess::PieceType::PAWN):   return 0;
            case static_cast<int>(chess::PieceType::KNIGHT): return 1;
            case static_cast<int>(chess::PieceType::BISHOP): return 2;
            case static_cast<int>(chess::PieceType::ROOK):   return 3;
            case static_cast<int>(chess::PieceType::QUEEN):  return 4;
            case static_cast<int>(chess::PieceType::KING):   return 5;
            default: return 0;
        }
    };

    PGNGame game = PGNLoader::load(pgnPath);

    chess::Board board;
    if (!game.fen.empty()) {
        board = chess::Board(game.fen);
    }

    for (int sq = 0; sq < 64; sq++) {
        chess::Piece piece = board.at(chess::Square(sq));
        if (piece == chess::Piece::NONE) continue;

        int file = sq % 8;
        int rank = sq / 8;
        int idx = pieceTemplateIndex(piece.type());

        Object& tmpl = (piece.color() == chess::Color::WHITE) ? whiteTemplates[idx] : blackTemplates[idx];
        grid[file][rank] = &placePiece(world, tmpl, fileToX(file), rankToY(rank));
    }

    Object boardObj = OBJLoader::loadObject("assets/chess/board.obj", "assets/chess/board.mtl");
    boardObj.transform.scale = glm::vec3(4.2f);
    world.addObject(boardObj);
    world.addGlobalLight(glm::vec3(0.5f, 1.0f, -1.0f));

    float moveDuration = 0.5f;
    float pauseDuration = 0.2f;
    Timeline& tl = world.getTimeline();

    for (const auto& san : game.sanMoves) {
        chess::Move move;
        try {
            move = chess::uci::parseSan(board, san);
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse SAN '" << san << "': " << e.what() << std::endl;
            continue;
        }

        if (move == chess::Move::NO_MOVE) {
            std::cerr << "No move for SAN: " << san << std::endl;
            continue;
        }

        bool isCapture = board.isCapture(move);
        bool isCastle = move.typeOf() == chess::Move::CASTLING;
        bool isEnPassant = move.typeOf() == chess::Move::ENPASSANT;
        bool isPromotion = move.typeOf() == chess::Move::PROMOTION;

        int fromSq = move.from().index();
        int toSq = move.to().index();
        int fromFile = fromSq % 8, fromRank = fromSq / 8;
        int toFile = toSq % 8, toRank = toSq / 8;

        if (isCastle) {
            bool kingSide = (move.to().file() == chess::File::FILE_H);
            int rank = fromRank;
            int rookFromF = kingSide ? 7 : 0;
            int rookToF   = kingSide ? 5 : 3;
            int kingToF   = kingSide ? 6 : 2;

            if (grid[fromFile][rank] && grid[rookFromF][rank]) {
                tl.parallel([&](Timeline& p) {
                    p.move(*grid[fromFile][rank], glm::vec3(fileToX(kingToF), rankToY(rank), Z_PIECE), moveDuration, Easing::easeInOutQuad);
                    p.move(*grid[rookFromF][rank], glm::vec3(fileToX(rookToF), rankToY(rank), Z_PIECE), moveDuration, Easing::easeInOutQuad);
                });
                grid[kingToF][rank] = grid[fromFile][rank];
                grid[rookToF][rank] = grid[rookFromF][rank];
                grid[fromFile][rank] = nullptr;
                grid[rookFromF][rank] = nullptr;
            }
        } else {
            if (isCapture && !isEnPassant) {
                if (grid[toFile][toRank]) {
                    tl.fadeOut(*grid[toFile][toRank], moveDuration, Easing::easeInQuad);
                }
            }

            if (isEnPassant) {
                int capturedRank = fromRank;
                if (grid[toFile][capturedRank]) {
                    tl.fadeOut(*grid[toFile][capturedRank], moveDuration, Easing::easeInQuad);
                    grid[toFile][capturedRank] = nullptr;
                }
            }

            if (grid[fromFile][fromRank]) {
                tl.move(*grid[fromFile][fromRank], glm::vec3(fileToX(toFile), rankToY(toRank), Z_PIECE), moveDuration, Easing::easeInOutSine);

                grid[toFile][toRank] = grid[fromFile][fromRank];
                grid[fromFile][fromRank] = nullptr;
            }

            if (isPromotion) {
                int idx = pieceTemplateIndex(move.promotionType());
                bool white = (board.sideToMove() == chess::Color::BLACK);
                Object& tmpl = white ? whiteTemplates[idx] : blackTemplates[idx];
                if (grid[toFile][toRank]) {
                    tl.hide(*grid[toFile][toRank]);
                }
                grid[toFile][toRank] = &placePiece(world, tmpl, fileToX(toFile), rankToY(toRank));
            }
        }

        board.makeMove(move);
        tl.wait(pauseDuration);
    }

    std::cout << "PGN animation loaded: " << game.sanMoves.size() << " half-moves" << std::endl;
}
