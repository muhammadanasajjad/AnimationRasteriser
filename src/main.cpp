#include <iostream>

#include <glm/glm.hpp>

#include <World.h>
#include <OBJLoader.h>

int main() {
    World world;
    if (!world.init(800, 800, "Animation Rasteriser")) {
        return -1;
    }

    Object car = OBJLoader::loadObject("assets/ferrari/ferrari.obj", "assets/ferrari/ferrari.mtl");
    world.addObject(car);

    world.addGlobalLight(glm::vec3(0.5f, 1.0f, -1.0f));
    world.run();
    world.cleanup();

    return 0;
}
