//
// Created by lasagnaphil on 2021-07-25.
//

#include "scene.h"
#include "engine.h"
#include "script.h"
#include "sound.h"

#include "core/log.h"
#include "render/sprite.h"
#include "render/tilemap.h"
#include "squirrel/utils.h"
#include "squirrel/vm.h"
#include "resource_pool.h"

#include "squirrel/binding.h"

#include <Tracy.hpp>

void Scene::load() {
    ZoneScoped
    auto res = this->engine->get_resources();
    auto vm = this->engine->get_vm();

    script_ref = vm->require_module(script_path.c_str(), true, nullptr);
    if (!script_ref) {
        log_fatal("Failed to load scene with script {}!", script_path);
        std::abort();
    }

    auto script = script_ref.get();

    // script->exports is scene class
    cls = sq::Class(script->exports);

    // Find methods
    sq_constructor = vm->get<sq::Function>(cls, "constructor");
    sq_on_load = vm->get<sq::Function>(cls, "on_load");
    sq_on_update = vm->get<sq::Function>(cls, "on_update");
    sq_on_render = vm->get<sq::Function>(cls, "on_render");

    inst = vm->new_instance_without_ctor(cls);
    vm->call_func(sq_constructor, inst);

    name = vm->get<std::string>(inst, "name");

    log_info("Loaded scene script '{}' with name '{}'!", script_path, name);

    // Push resource label
    std::string res_label_prefix = "scene/";
    res_label = make_res_label(res_label_prefix + name);
    res->push_label(res_label);

    // Load tilemap
    auto tmx_str = vm->get_or_default<std::string>(inst, "tmx", "");
    if (!tmx_str.empty()) {
        std::string tmx_filepath = std::string("scenes/") + tmx_str;
        this->tilemap_ref = Tilemap::load(tmx_filepath.c_str());
        load_tilemap_sprites();

        log_info("Loaded tilemap {}!", tmx_filepath.c_str());
    }

    // Call on_load
    vm->call_func(sq_on_load, inst);
}

void Scene::update(float dt) {
    ZoneScoped
    if (sq_on_update.is_null()) return;

    auto res = this->engine->get_resources();
    auto sqvm = this->engine->get_vm();
    auto vm = sqvm->handle();

    if (SQ_FAILED(sq::call_noreturn(vm, sq_on_update, inst, dt))) {
        log_error("Failed to call on_update for scene {}!", name);
        return;
    }
}

void Scene::render() {
    ZoneScoped
    if (sq_on_render.is_null()) return;

    auto res = this->engine->get_resources();
    auto sqvm = this->engine->get_vm();
    auto vm = sqvm->handle();
    auto camera = this->engine->get_camera();

    if (SQ_FAILED(sq::call_noreturn(vm, sq_on_render, inst))) {
        log_error("Failed to call on_update for scene {}!", name);
        return;
    }
}

void Scene::release() {
    ZoneScoped
    auto sqvm = this->engine->get_vm();
    auto vm = sqvm->handle();
    auto res = this->engine->get_resources();

    engine->get_sound()->stop_all();

    // sq_release(vm, &sq_constructor.obj);
    // sq_release(vm, &sq_on_load.obj);
    // sq_release(vm, &sq_on_update.obj);
    // sq_release(vm, &sq_on_render.obj);
    // sq_release(vm, &inst.obj);
    // sq_release(vm, &cls.obj);

    res->release_with_label(res_label);
    res->pop_label();
}

void Scene::load_tilemap_sprites() {
    ZoneScoped
    assert(tilemap_ref);
    tilemap_ref.get()->insert_to_scene();
}

void Scene::reload() {
    ZoneScoped
    release();
    engine->reset();
    load();
}
