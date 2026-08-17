#include <iostream>

#include <World.h>
#include <OBJLoader.h>

int main() {
    World world;
    if (!world.init(800, 800, "Animation Rasteriser")) {
        return -1;
    }

    Object car = OBJLoader::loadObject("assets/car.obj", "assets/car.mtl");
    world.addObject(car);

    world.run();
    world.cleanup();

    return 0;
}
