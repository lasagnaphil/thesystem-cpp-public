#include "engine.h"
#include "squirrel/vm.h"

class MyApp : public Engine {
public:
    MyApp(int argc, char** argv) : Engine(argc, argv) {}

    void pre_init() override {
        config_script = sqvm->require_module("config", true, nullptr);
        if (!config_script) {
            log_fatal("Failed to load config script!");
            std::abort();
        }
        auto cfg = sq::Table(config_script.get()->exports);
        auto resolution = sqvm->get<ivec2>(cfg, "resolution");
        game_width = resolution.x;
        game_height = resolution.y;

        scene_name = sqvm->get<std::string>(cfg, "scene");
    }

    void init() override {
        push_scene(scene_name.c_str());
    }

    void update(double dt) override {

    }

    void render() override {

    }

    void release() override {

    }

private:
    Ref<ScriptModule> config_script;
    std::string scene_name;
};

sapp_desc sokol_main(int argc, char **argv) {
    auto app = Engine::create<MyApp>(argc, argv);
    return app->get_app_desc();
}