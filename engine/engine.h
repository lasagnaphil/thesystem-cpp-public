//
// Created by lasagnaphil on 2021-07-25.
//

#ifndef THESYSTEM_ENGINE_H
#define THESYSTEM_ENGINE_H

#include "scene.h"

#include "core/rect.h"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_gfx_imgui.h"

#include <memory>
#include <array>

class VM;
class Resources;
class Input;
class Sound;
class Camera;
class SpriteRenderer;
class CollisionManager;

class Engine {
public:
    Engine(int argc, char** argv);
    virtual ~Engine();
    Engine(const Engine& engine) = delete;
    Engine& operator=(const Engine& engine) = delete;

    static Engine& instance() { return *inst; }
    template <class T>
    static T* create(int argc, char** argv) {
        if (inst) {
            printf("More than one instance of engine is created!\n");
            exit(EXIT_FAILURE);
        }
        auto engine = new T(argc, argv);
        inst = engine;
        inst->base_pre_init();
        return engine;
    }

    void push_scene(const char* scene_script_path);
    sapp_desc get_app_desc();

    VM* get_vm() { return sqvm.get(); }
    Resources* get_resources() { return res.get(); }
    Input* get_input() { return input.get(); }
    Sound* get_sound() { return sound.get(); }
    SpriteRenderer* get_sprite_renderer() { return sprite_renderer.get(); }
    Camera* get_camera() { return camera.get(); }
    CollisionManager* get_collision_manager() { return collision_manager.get(); }

    int get_fps() { return measured_avg_fps; }

    virtual void pre_init() = 0;
    virtual void init() = 0;
    virtual void update(double dt) = 0;
    virtual void render() = 0;
    virtual void release() = 0;

    void base_pre_init();
    void base_init();
    void base_update(double dt);
    void base_event(const sapp_event* e);
    void base_render();
    void base_release();

    void reset();

protected:
    static Engine* inst;

    int argc; char** argv;
    sapp_desc app_desc;
    sg_pass_action clear_pass_action;

    sg_imgui_t sg_imgui;
    bool show_debug_menu = false;

    bool needs_reload = false;

    int window_size_multiplier = 1;
    int game_width = 640;
    int game_height = 480;
    irect viewport_rect;

    std::unique_ptr<VM> sqvm;
    std::unique_ptr<Resources> res;
    std::unique_ptr<Input> input;
    std::unique_ptr<Sound> sound;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<SpriteRenderer> sprite_renderer;
    std::unique_ptr<CollisionManager> collision_manager;

    std::vector<Scene> scene_stack;

    uint64_t last_frame_time;
    double measured_dt, measured_avg_dt;
    int measured_fps, measured_avg_fps;
    std::array<double, 10> past_measured_dts;
};

#endif //THESYSTEM_ENGINE_H
