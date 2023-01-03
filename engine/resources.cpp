//
// Created by lasagnaphil on 2021-07-25.
//

#include "resources.h"

#include "reflected_headers.h"
#include "engine.h"
#include "squirrel/scriptable_impl.h"

Resources::Resources() {
    push_label(make_res_label("all"));
}

Resources::~Resources() {

}

void Resources::push_label(ResourceLabel label) {
    label_stack.push_back(label);
    set_labels_for_resource_pools(label);
}

void Resources::pop_label() {
    assert(label_stack.size() > 1);
    label_stack.pop_back();
    set_labels_for_resource_pools(label_stack.back());
}

void Resources::release_with_label(ResourceLabel label) {
    pool_Animation.release(label);
    pool_Animation_scriptable.release(label);
    pool_AudioInstance.release(label);
    pool_AudioSource.release(label);
    pool_Collider.release(label);
    pool_Collider_scriptable.release(label);
    pool_Font.release(label);
    pool_Image.release(label);
    pool_KinematicBody.release(label);
    pool_KinematicBody_scriptable.release(label);
    pool_Node.release(label);
    pool_Node_scriptable.release(label);
    pool_ScriptModule.release(label);
    pool_Sprite.release(label);
    pool_Sprite_scriptable.release(label);
    pool_Text.release(label);
    pool_Text_scriptable.release(label);
    pool_Texture.release(label);
    pool_Tilemap.release(label);
    pool_Tileset.release(label);
}

void Resources::set_labels_for_resource_pools(ResourceLabel label) {
    pool_Animation.set_resource_label(label);
    pool_Animation_scriptable.set_resource_label(label);
    pool_AudioInstance.set_resource_label(label);
    pool_AudioSource.set_resource_label(label);
    pool_Collider.set_resource_label(label);
    pool_Collider_scriptable.set_resource_label(label);
    pool_Font.set_resource_label(label);
    pool_Image.set_resource_label(label);
    pool_KinematicBody.set_resource_label(label);
    pool_KinematicBody_scriptable.set_resource_label(label);
    pool_Node.set_resource_label(label);
    pool_Node_scriptable.set_resource_label(label);
    pool_ScriptModule.set_resource_label(label);
    pool_Sprite.set_resource_label(label);
    pool_Sprite_scriptable.set_resource_label(label);
    pool_Text.set_resource_label(label);
    pool_Text_scriptable.set_resource_label(label);
    pool_Texture.set_resource_label(label);
    pool_Tilemap.set_resource_label(label);
    pool_Tileset.set_resource_label(label);
}

void Resources::scriptable_update(float dt) {
    auto vm = Engine::instance().get_vm();
    pool_Animation_scriptable.foreach([vm, dt](Scriptable<Animation>& script) {
        script.update(*vm, dt);
    });
    pool_Collider_scriptable.foreach([vm, dt](Scriptable<Collider>& script) {
        script.update(*vm, dt);
    });
    pool_KinematicBody_scriptable.foreach([vm, dt](Scriptable<KinematicBody>& script) {
        script.update(*vm, dt);
    });
    pool_Node_scriptable.foreach([vm, dt](Scriptable<Node>& script) {
        script.update(*vm, dt);
    });
    pool_Sprite_scriptable.foreach([vm, dt](Scriptable<Sprite>& script) {
        script.update(*vm, dt);
    });
    pool_Text_scriptable.foreach([vm, dt](Scriptable<Text>& script) {
        script.update(*vm, dt);
    });
}

void Resources::scriptable_render() {
    auto vm = Engine::instance().get_vm();
    pool_Animation_scriptable.foreach([vm](Scriptable<Animation>& script) {
        script.render(*vm);
    });
    pool_Collider_scriptable.foreach([vm](Scriptable<Collider>& script) {
        script.render(*vm);
    });
    pool_KinematicBody_scriptable.foreach([vm](Scriptable<KinematicBody>& script) {
        script.render(*vm);
    });
    pool_Node_scriptable.foreach([vm](Scriptable<Node>& script) {
        script.render(*vm);
    });
    pool_Sprite_scriptable.foreach([vm](Scriptable<Sprite>& script) {
        script.render(*vm);
    });
    pool_Text_scriptable.foreach([vm](Scriptable<Text>& script) {
        script.render(*vm);
    });
}