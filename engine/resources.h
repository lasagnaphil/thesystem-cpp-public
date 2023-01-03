//
// Created by lasagnaphil on 2021-07-25.
//

#ifndef THESYSTEM_RESOURCES_H
#define THESYSTEM_RESOURCES_H

#include "resource_pool.h"

#include <vector>
#include <string>
#include <unordered_map>

#include "squirrel/scriptable.h"

// Forward declaration
class Animation;
class AudioInstance;
class AudioSource;
class Collider;
class Font;
class Image;
class KinematicBody;
class Node;
class Options;
class ScriptModule;
class Sprite;
class Text;
class Texture;
class Tilemap;
class Tileset;

template <class T>
class Scriptable;

template <class T>
struct is_resource {
    static constexpr bool value = false;
};

template <class T>
inline constexpr bool is_resource_v = is_resource<T>::value;

template <template <class...> class C, class T>
struct is_resource<C<T>> {
    static constexpr bool value = std::is_same_v<C<T>, Scriptable<T>> && is_resource_v<T>;
};

template <>
struct is_resource<Animation> {
    static constexpr bool value = true;
};
template <>
struct is_resource<AudioInstance> {
    static constexpr bool value = true;
};
template <>
struct is_resource<AudioSource> {
    static constexpr bool value = true;
};
template <>
struct is_resource<Collider> {
    static constexpr bool value = true;
};
template <>
struct is_resource<Font> {
    static constexpr bool value = true;
};
template <>
struct is_resource<Image> {
    static constexpr bool value = true;
};
template <>
struct is_resource<KinematicBody> {
    static constexpr bool value = true;
};
template <>
struct is_resource<Node> {
    static constexpr bool value = true;
};
template <>
struct is_resource<ScriptModule> {
    static constexpr bool value = true;
};
template <>
struct is_resource<Sprite> {
    static constexpr bool value = true;
};
template <>
struct is_resource<Text> {
    static constexpr bool value = true;
};
template <>
struct is_resource<Texture> {
    static constexpr bool value = true;
};
template <>
struct is_resource<Tilemap> {
    static constexpr bool value = true;
};
template <>
struct is_resource<Tileset> {
    static constexpr bool value = true;
};

class Resources {

public:
    Resources();
    ~Resources();

    void push_label(ResourceLabel label);

    void pop_label();

    void release_with_label(ResourceLabel label);

    template <class T>
    StableResourcePool<T>& get_pool();

    template <>
    StableResourcePool<Animation>& get_pool<Animation>() { return pool_Animation; }
    template <>
    StableResourcePool<Scriptable<Animation>>& get_pool<Scriptable<Animation>>() { return pool_Animation_scriptable; }
    template <>
    StableResourcePool<AudioInstance>& get_pool<AudioInstance>() { return pool_AudioInstance; }
    template <>
    StableResourcePool<AudioSource>& get_pool<AudioSource>() { return pool_AudioSource; }
    template <>
    StableResourcePool<Collider>& get_pool<Collider>() { return pool_Collider; }
    template <>
    StableResourcePool<Scriptable<Collider>>& get_pool<Scriptable<Collider>>() { return pool_Collider_scriptable; }
    template <>
    StableResourcePool<Font>& get_pool<Font>() { return pool_Font; }
    template <>
    StableResourcePool<Image>& get_pool<Image>() { return pool_Image; }
    template <>
    StableResourcePool<KinematicBody>& get_pool<KinematicBody>() { return pool_KinematicBody; }
    template <>
    StableResourcePool<Scriptable<KinematicBody>>& get_pool<Scriptable<KinematicBody>>() { return pool_KinematicBody_scriptable; }
    template <>
    StableResourcePool<Node>& get_pool<Node>() { return pool_Node; }
    template <>
    StableResourcePool<Scriptable<Node>>& get_pool<Scriptable<Node>>() { return pool_Node_scriptable; }
    template <>
    StableResourcePool<ScriptModule>& get_pool<ScriptModule>() { return pool_ScriptModule; }
    template <>
    StableResourcePool<Sprite>& get_pool<Sprite>() { return pool_Sprite; }
    template <>
    StableResourcePool<Scriptable<Sprite>>& get_pool<Scriptable<Sprite>>() { return pool_Sprite_scriptable; }
    template <>
    StableResourcePool<Text>& get_pool<Text>() { return pool_Text; }
    template <>
    StableResourcePool<Scriptable<Text>>& get_pool<Scriptable<Text>>() { return pool_Text_scriptable; }
    template <>
    StableResourcePool<Texture>& get_pool<Texture>() { return pool_Texture; }
    template <>
    StableResourcePool<Tilemap>& get_pool<Tilemap>() { return pool_Tilemap; }
    template <>
    StableResourcePool<Tileset>& get_pool<Tileset>() { return pool_Tileset; }

    template <class T, class ...Args>
    Ref<T> new_item(Args&&... args) { return get_pool<T>().new_item(std::forward<Args>(args)...); }

    template <class T, class Fun>
    void foreach(Fun&& fun) {
        if constexpr (std::is_void_v<T>) {
            pool_Animation.foreach(fun);
            pool_AudioInstance.foreach(fun);
            pool_AudioSource.foreach(fun);
            pool_Collider.foreach(fun);
            pool_Font.foreach(fun);
            pool_Image.foreach(fun);
            pool_KinematicBody.foreach(fun);
            pool_Node.foreach(fun);
            pool_ScriptModule.foreach(fun);
            pool_Sprite.foreach(fun);
            pool_Text.foreach(fun);
            pool_Texture.foreach(fun);
            pool_Tilemap.foreach(fun);
            pool_Tileset.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, Animation>) {
            pool_Animation.foreach(fun);
            pool_Animation_scriptable.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, AudioInstance>) {
            pool_AudioInstance.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, AudioSource>) {
            pool_AudioSource.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, Collider>) {
            pool_Collider.foreach(fun);
            pool_Collider_scriptable.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, Font>) {
            pool_Font.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, Image>) {
            pool_Image.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, KinematicBody>) {
            pool_KinematicBody.foreach(fun);
            pool_KinematicBody_scriptable.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, Node>) {
            pool_Node.foreach(fun);
            pool_Node_scriptable.foreach(fun);
            pool_Collider.foreach(fun);
            pool_Collider_scriptable.foreach(fun);
            pool_KinematicBody.foreach(fun);
            pool_KinematicBody_scriptable.foreach(fun);
            pool_Sprite.foreach(fun);
            pool_Sprite_scriptable.foreach(fun);
            pool_Text.foreach(fun);
            pool_Text_scriptable.foreach(fun);
            pool_Animation.foreach(fun);
            pool_Animation_scriptable.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, ScriptModule>) {
            pool_ScriptModule.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, Sprite>) {
            pool_Sprite.foreach(fun);
            pool_Sprite_scriptable.foreach(fun);
            pool_Animation.foreach(fun);
            pool_Animation_scriptable.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, Text>) {
            pool_Text.foreach(fun);
            pool_Text_scriptable.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, Texture>) {
            pool_Texture.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, Tilemap>) {
            pool_Tilemap.foreach(fun);
        }
        else if constexpr (std::is_same_v<T, Tileset>) {
            pool_Tileset.foreach(fun);
        }
    }

    template <class T, class Fun>
    void foreach_ref(Fun&& fun) {
        if constexpr (std::is_void_v<T>) {
            pool_Animation.foreach_ref(fun);
            pool_AudioInstance.foreach_ref(fun);
            pool_AudioSource.foreach_ref(fun);
            pool_Collider.foreach_ref(fun);
            pool_Font.foreach_ref(fun);
            pool_Image.foreach_ref(fun);
            pool_KinematicBody.foreach_ref(fun);
            pool_Node.foreach_ref(fun);
            pool_ScriptModule.foreach_ref(fun);
            pool_Sprite.foreach_ref(fun);
            pool_Text.foreach_ref(fun);
            pool_Texture.foreach_ref(fun);
            pool_Tilemap.foreach_ref(fun);
            pool_Tileset.foreach_ref(fun);
        }
        else if constexpr (std::is_same_v<T, Animation>) {
            pool_Animation.foreach_ref(fun);
            pool_Animation_scriptable.foreach_ref<Animation>(fun);
        }
        else if constexpr (std::is_same_v<T, AudioInstance>) {
            pool_AudioInstance.foreach_ref(fun);
        }
        else if constexpr (std::is_same_v<T, AudioSource>) {
            pool_AudioSource.foreach_ref(fun);
        }
        else if constexpr (std::is_same_v<T, Collider>) {
            pool_Collider.foreach_ref(fun);
            pool_Collider_scriptable.foreach_ref<Collider>(fun);
        }
        else if constexpr (std::is_same_v<T, Font>) {
            pool_Font.foreach_ref(fun);
        }
        else if constexpr (std::is_same_v<T, Image>) {
            pool_Image.foreach_ref(fun);
        }
        else if constexpr (std::is_same_v<T, KinematicBody>) {
            pool_KinematicBody.foreach_ref(fun);
            pool_KinematicBody_scriptable.foreach_ref<KinematicBody>(fun);
        }
        else if constexpr (std::is_same_v<T, Node>) {
            pool_Node.foreach_ref(fun);
            pool_Node_scriptable.foreach_ref<Node>(fun);
            pool_Collider.foreach_ref<Node>(fun);
            pool_Collider_scriptable.foreach_ref<Node>(fun);
            pool_KinematicBody.foreach_ref<Node>(fun);
            pool_KinematicBody_scriptable.foreach_ref<Node>(fun);
            pool_Sprite.foreach_ref<Node>(fun);
            pool_Sprite_scriptable.foreach_ref<Node>(fun);
            pool_Text.foreach_ref<Node>(fun);
            pool_Text_scriptable.foreach_ref<Node>(fun);
            pool_Animation.foreach_ref<Node>(fun);
            pool_Animation_scriptable.foreach_ref<Node>(fun);
        }
        else if constexpr (std::is_same_v<T, ScriptModule>) {
            pool_ScriptModule.foreach_ref(fun);
        }
        else if constexpr (std::is_same_v<T, Sprite>) {
            pool_Sprite.foreach_ref(fun);
            pool_Sprite_scriptable.foreach_ref<Sprite>(fun);
            pool_Animation.foreach_ref<Sprite>(fun);
            pool_Animation_scriptable.foreach_ref<Sprite>(fun);
        }
        else if constexpr (std::is_same_v<T, Text>) {
            pool_Text.foreach_ref(fun);
            pool_Text_scriptable.foreach_ref<Text>(fun);
        }
        else if constexpr (std::is_same_v<T, Texture>) {
            pool_Texture.foreach_ref(fun);
        }
        else if constexpr (std::is_same_v<T, Tilemap>) {
            pool_Tilemap.foreach_ref(fun);
        }
        else if constexpr (std::is_same_v<T, Tileset>) {
            pool_Tileset.foreach_ref(fun);
        }
    }

    void scriptable_update(float dt);
    void scriptable_render();

private:
    void set_labels_for_resource_pools(ResourceLabel label);

    std::vector<ResourceLabel> label_stack;

    // Resource pools
    StableResourcePool<Animation> pool_Animation;
    StableResourcePool<Scriptable<Animation>> pool_Animation_scriptable;
    StableResourcePool<AudioInstance> pool_AudioInstance;
    StableResourcePool<AudioSource> pool_AudioSource;
    StableResourcePool<Collider> pool_Collider;
    StableResourcePool<Scriptable<Collider>> pool_Collider_scriptable;
    StableResourcePool<Font> pool_Font;
    StableResourcePool<Image> pool_Image;
    StableResourcePool<KinematicBody> pool_KinematicBody;
    StableResourcePool<Scriptable<KinematicBody>> pool_KinematicBody_scriptable;
    StableResourcePool<Node> pool_Node;
    StableResourcePool<Scriptable<Node>> pool_Node_scriptable;
    StableResourcePool<ScriptModule> pool_ScriptModule;
    StableResourcePool<Sprite> pool_Sprite;
    StableResourcePool<Scriptable<Sprite>> pool_Sprite_scriptable;
    StableResourcePool<Text> pool_Text;
    StableResourcePool<Scriptable<Text>> pool_Text_scriptable;
    StableResourcePool<Texture> pool_Texture;
    StableResourcePool<Tilemap> pool_Tilemap;
    StableResourcePool<Tileset> pool_Tileset;

};

#endif //THESYSTEM_RESOURCES_H