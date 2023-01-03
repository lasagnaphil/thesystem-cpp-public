//
// Created by lasagnaphil on 2021-07-31.
//

#ifndef THESYSTEM_TILEMAP_H
#define THESYSTEM_TILEMAP_H

#include <string>
#include <vector>
#include <unordered_map>

#include "resource_pool.h"
#include "core/rect.h"
#include "core/reflect.h"
#include "core/color.h"

#include "collision/collider.h"

class Engine;

struct TilesetObject {
    std::string source;
    int width = 0, height = 0;
    std::vector<Collider::Options> colliders;
};

CLASS(Resource) Tileset {
public:
    std::string filename;
    std::string version;
    std::string tiledversion;
    std::string name;
    int tilewidth = 0, tileheight = 0;
    int tilecount = 0, columns = 0;

    enum class Type {
        Image, Object
    } type;

    Ref<Texture> image_tex_ref;
    std::string image_source;
    int image_width = 0, image_height = 0;

    std::unordered_map<int, TilesetObject> objects;

    static Ref<Tileset> load(const char* filename);
};

struct TilesetRef {
    int firstgid;
    int count;
    Ref<Tileset> ref;
};

enum class TiledPropertyType {
    Bool,
    Color,
    Float,
    File,
    Int,
    Object,
    String
};

struct TiledPropertyValue {
    TiledPropertyType type;
    std::string value_string;
    union {
        bool value_bool;
        rgba value_color;
        float value_float;
        int value_int;
    };
};

enum class TiledObjectType {
    Rectangle, Point, Ellipse, Polygon, Tile, Template, Text
};

struct TiledObject {
    // TiledObjectType type;

    int id = 0;
    int gid = 0;
    std::string tmpl;
    std::string name;
    std::string type;
    float x = 0, y = 0;
    float width = 0, height = 0;
    float rotation = 0;

    Ref<ScriptModule> script;
    std::unordered_map<std::string, TiledPropertyValue> properties;
};

struct TiledObjectTemplate {
    TiledObject obj;

    int tileset_firstgid;
    std::string tileset_source;
};

struct TiledLayer {
    std::string name;
    int id = 0;

    enum class Type {
        Tile, ObjectGroup
    } type;

    // Tile type
    int width, height;
    std::vector<uint16_t> data;

    // Object type
    std::vector<TiledObject> objects;
    std::unordered_map<std::string, uint32_t> object_names;

    TiledObject get_object(const char* obj_name) {
        assert(type == Type::ObjectGroup);
        return objects[object_names.at(obj_name)];
    }
};

struct TiledLayerGroup {
    std::string name;
    int id = 0;
    std::vector<TiledLayer> layers;
};

namespace pugi {
    class xml_node;
}

CLASS(Resource) Tilemap {
public:
    std::string version;
    std::string tiledversion;
    int width, height;
    int tilewidth, tileheight;
    bool infinite;

    std::vector<TilesetRef> tilesets;
    std::vector<TiledLayerGroup> layer_groups;
    std::unordered_map<std::string, TiledObjectTemplate> templates;

    int player_layer_idx = 0;

    static Ref<Tilemap> load(const char* filename);

    void insert_to_scene();

private:

    TiledObjectTemplate* load_template_object(const std::string& filepath);

    TiledObject parse_object(const pugi::xml_node& node);
    TiledObjectTemplate parse_template_object(const pugi::xml_node& node);
    TiledLayer parse_layer(const pugi::xml_node& node);
};

#endif //THESYSTEM_TILEMAP_H
