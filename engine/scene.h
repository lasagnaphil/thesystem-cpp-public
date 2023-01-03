//
// Created by lasagnaphil on 2021-07-25.
//

#ifndef THESYSTEM_SCENE_H
#define THESYSTEM_SCENE_H

#include "resource_pool.h"
#include "squirrel/object.h"
#include "squirrel/script_module.h"

#include <string>
#include <vector>

class Engine;
class Tilemap;

class Scene {
    Engine* engine;
    std::string script_path;
    Ref<ScriptModule> script_ref = {};

    sq::Function sq_constructor;
    sq::Function sq_on_load;
    sq::Function sq_on_update;
    sq::Function sq_on_render;
    sq::Class cls;
    sq::Instance inst;

    std::string name;
    ResourceLabel res_label;

    Ref<Tilemap> tilemap_ref = {};

public:
    Scene(Engine* engine, const char* scene_script_path) : engine(engine), script_path(scene_script_path) {}

    std::string get_script_path() { return script_path; }

    void load();

    void update(float dt);
    void render();

    void release();

    void reload();

    void load_tilemap_sprites();
};

#endif //THESYSTEM_SCENE_H
