#pragma once

#include <cstdint>
#include <string_view>

static constexpr uint32_t offset = 2166136261u;
static constexpr uint32_t prime = 16777619u;

constexpr uint32_t hash_str_helper(uint32_t partial, const char* str) {
    return str[0] == 0? partial : hash_str_helper((partial^str[0])*prime, str+1);
}

constexpr uint32_t hash_str(const char* str) {
    return hash_str_helper(offset, str);
}

template <class T>
struct typeinfo {
    using base_type = void;
    static constexpr std::string_view name = "";
    static constexpr uint32_t id = 0;
    static constexpr uint32_t descendant_lookup_table[16] = {false};
};

template <>
struct typeinfo<void> {
    using base_type = void;
    static constexpr std::string_view name = "";
    static constexpr uint32_t id = 0;
    static constexpr uint32_t descendant_lookup_table[16] = {false};
};

template <template <class...> class C, class T>
struct typeinfo<C<T>> {
    using base_type = typename typeinfo<T>::base_type;
    static constexpr std::string_view name = typeinfo<T>::name;
    static constexpr uint32_t id = typeinfo<T>::id;
    static constexpr uint32_t descendant_lookup_table[16] = typeinfo<T>::descendant_lookup_table;
};

class Animation;
class AudioInstance;
class AudioSource;
class Collider;
class Font;
class Image;
class KinematicBody;
class Node;
class ScriptModule;
class Sprite;
class Text;
class Texture;
class Tilemap;
class Tileset;

template <>
struct typeinfo<Animation> {
    using base_type = Sprite;
    static constexpr std::string_view name = "Animation";
    static constexpr uint32_t id = 1;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,true,false,false,false,false,false,false,false,false,false,false,false,false,false,false
    };
};
template <>
struct typeinfo<AudioInstance> {
    using base_type = void;
    static constexpr std::string_view name = "AudioInstance";
    static constexpr uint32_t id = 2;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,false,true,false,false,false,false,false,false,false,false,false,false,false,false,false
    };
};
template <>
struct typeinfo<AudioSource> {
    using base_type = void;
    static constexpr std::string_view name = "AudioSource";
    static constexpr uint32_t id = 3;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,false,false,true,false,false,false,false,false,false,false,false,false,false,false,false
    };
};
template <>
struct typeinfo<Collider> {
    using base_type = Node;
    static constexpr std::string_view name = "Collider";
    static constexpr uint32_t id = 4;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,false,false,false,true,false,false,false,false,false,false,false,false,false,false,false
    };
};
template <>
struct typeinfo<Font> {
    using base_type = void;
    static constexpr std::string_view name = "Font";
    static constexpr uint32_t id = 5;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,false,false,false,false,true,false,false,false,false,false,false,false,false,false,false
    };
};
template <>
struct typeinfo<Image> {
    using base_type = void;
    static constexpr std::string_view name = "Image";
    static constexpr uint32_t id = 6;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,false,false,false,false,false,true,false,false,false,false,false,false,false,false,false
    };
};
template <>
struct typeinfo<KinematicBody> {
    using base_type = Node;
    static constexpr std::string_view name = "KinematicBody";
    static constexpr uint32_t id = 7;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,false,false,false,false,false,false,true,false,false,false,false,false,false,false,false
    };
};
template <>
struct typeinfo<Node> {
    using base_type = void;
    static constexpr std::string_view name = "Node";
    static constexpr uint32_t id = 8;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,true,false,false,true,false,false,true,true,false,false,true,true,false,false,false
    };
};
template <>
struct typeinfo<ScriptModule> {
    using base_type = void;
    static constexpr std::string_view name = "ScriptModule";
    static constexpr uint32_t id = 10;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false
    };
};
template <>
struct typeinfo<Sprite> {
    using base_type = Node;
    static constexpr std::string_view name = "Sprite";
    static constexpr uint32_t id = 11;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,true,false,false,false,false,false,false,false,false,false,true,false,false,false,false
    };
};
template <>
struct typeinfo<Text> {
    using base_type = Node;
    static constexpr std::string_view name = "Text";
    static constexpr uint32_t id = 12;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false
    };
};
template <>
struct typeinfo<Texture> {
    using base_type = void;
    static constexpr std::string_view name = "Texture";
    static constexpr uint32_t id = 13;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false
    };
};
template <>
struct typeinfo<Tilemap> {
    using base_type = void;
    static constexpr std::string_view name = "Tilemap";
    static constexpr uint32_t id = 14;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,false,false,false,false,false,false,false,false,false,false,false,false,false,true,false
    };
};
template <>
struct typeinfo<Tileset> {
    using base_type = void;
    static constexpr std::string_view name = "Tileset";
    static constexpr uint32_t id = 15;

    static constexpr uint32_t descendant_lookup_table[16] = {
            false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,true
    };
};

template <class T> inline constexpr std::string_view type_name(void) { return typeinfo<T>::name; }
template <class T> inline constexpr std::string_view type_name(const T& type) { return typeinfo<T>::name; }

template <class T>
constexpr uint32_t type_id() { return typeinfo<T>::id; }

template <class T>
constexpr uint32_t parent_type_id() { return typeinfo<typename typeinfo<T>::base_type>::parent_id; }

template <class T>
using base_type_v = typename typeinfo<T>::base_type;

#define REFLECTION_REGISTER_TYPE(_Type, _BaseType, _tid) \
template <> \
struct typeinfo<_Type> { \
    using base_type = _BaseType; \
    static constexpr std::string_view name = #_Type; \
    static constexpr uint32_t id = _tid; \
};