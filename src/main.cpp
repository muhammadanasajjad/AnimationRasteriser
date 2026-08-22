#include <iostream>
#include <string>

#include <World.h>
#include <chessSimpleAnimation.h>

int main(int argc, char* argv[]) {
    World world;
    if (!world.init(800, 800, "Animation Rasteriser")) {
        return -1;
    }

    bool recordVideo = false;
    std::string pgnPath;
    bool usePGN = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--record") {
            recordVideo = true;
        } else if (arg == "--pgn" && i + 1 < argc) {
            usePGN = true;
            pgnPath = argv[i + 1];
            i++;
        }
    }

    if (recordVideo) {
        world.enableVideoExport("output.mp4", 30);
    }

    if (usePGN) {
        chessPGNAnimation(world, pgnPath);
    } else {
        chessSimpleAnimation(world);
    }

    world.run();
    world.cleanup();
    return 0;
}
