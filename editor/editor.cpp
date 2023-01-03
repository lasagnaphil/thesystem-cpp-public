#include "engine.h"

int main(int argc, char **argv) {
    Engine engine;
    engine.init(argc, argv);
    engine.push_scene("test");
    engine.run();
    engine.release();

    return 0;
}