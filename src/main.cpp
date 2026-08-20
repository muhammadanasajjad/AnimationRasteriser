#include <iostream>

#include <glm/glm.hpp>

#include <World.h>
#include <OBJLoader.h>

static void placePiece(World& world, Object& piece, float x, float y) {
    Object copy = piece;
    copy.transform.rotation = glm::vec3(180.0f, 0.0f, 0.0f);
    copy.transform.position = glm::vec3(x, y, -5.0f);
    world.addObject(copy);
}

int main() {
    World world;
    if (!world.init(800, 800, "Animation Rasteriser")) {
        return -1;
    }

    Object white_rook   = OBJLoader::loadObject("assets/chess/pieces/white_rook.obj", "assets/chess/pieces/white_rook.mtl");
    Object white_knight = OBJLoader::loadObject("assets/chess/pieces/white_knight.obj", "assets/chess/pieces/white_knight.mtl");
    Object white_bishop = OBJLoader::loadObject("assets/chess/pieces/white_bishop.obj", "assets/chess/pieces/white_bishop.mtl");
    Object white_queen  = OBJLoader::loadObject("assets/chess/pieces/white_queen.obj", "assets/chess/pieces/white_queen.mtl");
    Object white_king   = OBJLoader::loadObject("assets/chess/pieces/white_king.obj", "assets/chess/pieces/white_king.mtl");
    Object white_pawn   = OBJLoader::loadObject("assets/chess/pieces/white_pawn.obj", "assets/chess/pieces/white_pawn.mtl");

    Object black_rook   = OBJLoader::loadObject("assets/chess/pieces/black_rook.obj", "assets/chess/pieces/black_rook.mtl");
    Object black_knight = OBJLoader::loadObject("assets/chess/pieces/black_knight.obj", "assets/chess/pieces/black_knight.mtl");
    Object black_bishop = OBJLoader::loadObject("assets/chess/pieces/black_bishop.obj", "assets/chess/pieces/black_bishop.mtl");
    Object black_queen  = OBJLoader::loadObject("assets/chess/pieces/black_queen.obj", "assets/chess/pieces/black_queen.mtl");
    Object black_king   = OBJLoader::loadObject("assets/chess/pieces/black_king.obj", "assets/chess/pieces/black_king.mtl");
    Object black_pawn   = OBJLoader::loadObject("assets/chess/pieces/black_pawn.obj", "assets/chess/pieces/black_pawn.mtl");

    float rookPos = 20.0f;
    float spacing = (2.0f * rookPos) / 7.0f;

    // pawns across all 8 columns
    for (int col = 0; col < 8; col++) {
        float x = -rookPos + col * spacing;
        placePiece(world, white_pawn, x, -rookPos + spacing);
        placePiece(world, black_pawn, x,  rookPos - spacing);
    }

    // white back rank: R N B Q K B N R
    placePiece(world, white_rook,   -rookPos + 0 * spacing, -rookPos);
    placePiece(world, white_knight, -rookPos + 1 * spacing, -rookPos);
    placePiece(world, white_bishop, -rookPos + 2 * spacing, -rookPos);
    placePiece(world, white_queen,  -rookPos + 3 * spacing, -rookPos);
    placePiece(world, white_king,   -rookPos + 4 * spacing, -rookPos);
    placePiece(world, white_bishop, -rookPos + 5 * spacing, -rookPos);
    placePiece(world, white_knight, -rookPos + 6 * spacing, -rookPos);
    placePiece(world, white_rook,   -rookPos + 7 * spacing, -rookPos);

    // black back rank: R N B Q K B N R
    placePiece(world, black_rook,   -rookPos + 0 * spacing,  rookPos);
    placePiece(world, black_knight, -rookPos + 1 * spacing,  rookPos);
    placePiece(world, black_bishop, -rookPos + 2 * spacing,  rookPos);
    placePiece(world, black_queen,  -rookPos + 3 * spacing,  rookPos);
    placePiece(world, black_king,   -rookPos + 4 * spacing,  rookPos);
    placePiece(world, black_bishop, -rookPos + 5 * spacing,  rookPos);
    placePiece(world, black_knight, -rookPos + 6 * spacing,  rookPos);
    placePiece(world, black_rook,   -rookPos + 7 * spacing,  rookPos);
    
    Object board = OBJLoader::loadObject("assets/chess/board.obj", "assets/chess/board.mtl");
    board.transform.scale = glm::vec3(4.2f);
    world.addObject(board);

    world.addGlobalLight(glm::vec3(0.5f, 1.0f, -1.0f));
    world.run();
    world.cleanup();

    return 0;
}
