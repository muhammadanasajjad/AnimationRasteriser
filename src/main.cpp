#include <iostream>
#include <string>

#include <World.h>
#include <Irt.h>
#include <chessSimpleAnimation.h>

namespace {

std::string trim(const std::string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

char selectDemo() {
    std::cout << "\nAnimation Rasteriser demos\n"
              << "  A) Single pawn animation\n"
              << "  B) Chess game from PGN (assets/chess/game.pgn)\n"
              << "  C) Ferrari turntable (assets/scenes/HelloWorld.irt)\n"
              << "  D) Layered 2D shapes (assets/scenes/LayerDemo.irt)\n"
              << "  E) 3D shapes: sphere, cylinder, OBJ (assets/scenes/ShapesDemo.irt)\n"
              << "  F) Square + parametric curve (assets/scenes/someScene.irt)\n"
              << "  G) Bezier path follow with depth (assets/scenes/PathDemo.irt)\n";

    while (true) {
        std::cout << "Select a demo [A-G] (A): " << std::flush;

        std::string line;
        if (!std::getline(std::cin, line)) return 'A';

        std::string choice = trim(line);
        if (choice.empty()) return 'A';

        char option = static_cast<char>(std::toupper(static_cast<unsigned char>(choice[0])));
        if (option >= 'A' && option <= 'G') return option;

        std::cout << "Unknown demo '" << choice << "', please choose A to G.\n";
    }
}

}

int main(int argc, char* argv[]) {
    World world;
    if (!world.init(800, 800, "Animation Rasteriser")) {
        return -1;
    }

    bool recordVideo = false;
    bool usePGN = false;
    std::string pgnPath;
    std::string scenePath;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--record") {
            recordVideo = true;
        } else if (arg == "--pgn" && i + 1 < argc) {
            usePGN = true;
            pgnPath = argv[i + 1];
            i++;
        } else if (arg.size() > 4 && arg.rfind(".irt") == arg.size() - 4) {
            scenePath = arg;
        }
    }

    if (scenePath.empty() && !usePGN) {
        char demo = static_cast<char>(std::toupper(static_cast<unsigned char>(selectDemo())));
        if (demo == 'B') {
            usePGN = true;
            pgnPath = "assets/chess/game.pgn";
        } else if (demo == 'C') {
            scenePath = "assets/scenes/HelloWorld.irt";
        } else if (demo == 'D') {
            scenePath = "assets/scenes/LayerDemo.irt";
        } else if (demo == 'E') {
            scenePath = "assets/scenes/ShapesDemo.irt";
        } else if (demo == 'F') {
            scenePath = "assets/scenes/someScene.irt";
        } else if (demo == 'G') {
            scenePath = "assets/scenes/PathDemo.irt";
        }
    }

    if (recordVideo) {
        world.enableVideoExport("output.mp4", 30);
    }

    try {
        if (!scenePath.empty()) {
            irt::playSceneFile(world, scenePath);
        } else if (usePGN) {
            chessPGNAnimation(world, pgnPath);
        } else {
            chessSimpleAnimation(world);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        world.cleanup();
        return -1;
    }

    world.run();
    world.cleanup();
    return 0;
}
