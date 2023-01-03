//
// Created by lasagnaphil on 2021-07-31.
//

#include "tilemap.h"

#include "engine.h"
#include "script.h"
#include "sprite.h"
#include "core/file.h"
#include "core/strparse.h"
#include "core/log.h"
#include "render/texture.h"
#include "squirrel/vm.h"
#include "collision/collision_manager.h"
#include "collision/kinematic_body.h"

#include "pugixml.hpp"

Ref<Tileset> Tileset::load(const char* filename) {
    std::string path = filename;
    std::vector<char> buf = load_file_to_buffer(filename);
    if (buf.empty()) return {};
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(buf.data(), buf.size());

    Tileset ts;
    auto node_tileset = doc.child("tileset");
    ts.filename = strip_relative_path(path);
    ts.version = node_tileset.attribute("version").value();
    ts.tiledversion = node_tileset.attribute("tiledversion").value();
    ts.name = node_tileset.attribute("name").value();
    ts.tilewidth = node_tileset.attribute("tilewidth").as_int();
    ts.tileheight = node_tileset.attribute("tileheight").as_int();
    ts.tilecount = node_tileset.attribute("tilecount").as_int();
    ts.columns = node_tileset.attribute("columns").as_int();

    auto node_image = node_tileset.child("image");
    if (node_image) {
        ts.type = Type::Image;
        ts.image_source = node_image.attribute("source").value();
        ts.image_tex_ref = Texture::from_image_file(ts.image_source.c_str(), 4);
        ts.image_width = node_image.attribute("width").as_int();
        ts.image_height = node_image.attribute("height").as_int();
    }
    else {
        ts.type = Type::Object;
        ts.image_tex_ref = {};
        ts.image_width = 0;
        ts.image_height = 0;
    }

    for (auto node_tile : node_tileset.children("tile")) {
        int id = node_tile.attribute("id").as_int();
        TilesetObject obj;
        auto node_image = node_tile.child("image");
        obj.width = node_image.attribute("width").as_int();
        obj.height = node_image.attribute("height").as_int();
        obj.source = node_image.attribute("source").as_string();
        auto node_objectgroup = node_tile.child("objectgroup");
        for (auto node_object : node_objectgroup.children("object")) {
            rect rect;
            Collider::Options opt;
            opt.type = ColliderType::AABB;
            opt.aabb.extents.x = 0.5f * ((float)node_object.attribute("width").as_int() - 1e-4f);
            opt.aabb.extents.y = 0.5f * ((float)node_object.attribute("height").as_int() - 1e-4f);
            opt.pos.x = node_object.attribute("x").as_int() + opt.aabb.extents.x;
            opt.pos.y = node_object.attribute("y").as_int() + opt.aabb.extents.y;
            opt.rot = 0;
            obj.colliders.push_back(opt);
        }
        ts.objects.insert({id, obj});
    }

    auto res = Engine::instance().get_resources();
    return res->get_pool<Tileset>().insert(ts);
}

TiledObject Tilemap::parse_object(const pugi::xml_node &node) {
    auto vm = Engine::instance().get_vm();
    TiledObject obj;
    obj.tmpl = node.attribute("template").value();
    if (!obj.tmpl.empty()) {
        auto tmpl = load_template_object(obj.tmpl);
        obj = tmpl->obj;
        obj.id = node.attribute("id").as_int(tmpl->obj.id);
        obj.gid = node.attribute("gid").as_int(tmpl->obj.gid);

        // TODO: fix O(n) lookup
        for (auto& tileset : tilesets) {
            auto tileset_filename = tileset.ref.get()->filename;
            if (tileset_filename == tmpl->tileset_source) {
                obj.gid += (tileset.firstgid - 1);
                break;
            }
        }
    }
    else {
        obj.id = node.attribute("id").as_int();
        obj.gid = node.attribute("gid").as_int();
    }

    if (node.attribute("name")) obj.name = node.attribute("name").value();
    if (node.attribute("type")) obj.type = node.attribute("type").value();
    if (node.attribute("x")) obj.x = node.attribute("x").as_float();
    if (node.attribute("y")) obj.y = node.attribute("y").as_float();
    if (node.attribute("width")) obj.width = node.attribute("width").as_float();
    if (node.attribute("height")) obj.height = node.attribute("height").as_float();
    if (node.attribute("rotation")) obj.rotation = node.attribute("rotation").as_float();
    auto node_properties = node.child("properties");
    if (node_properties) {
        for (auto node_property : node_properties) {
            TiledPropertyValue value;
            auto prop_name = node_property.attribute("name").value();
            if (strcmp(prop_name, "script") == 0) {
                assert(strcmp(node_property.attribute("type").value(), "file") == 0);
                auto script_path = strip_relative_path(node_property.attribute("value").value());
                obj.script = vm->require_module(script_path.c_str(), true, nullptr);
            }
            auto prop_type_str = node_property.attribute("type").value();
            auto prop_value = node_property.attribute("value");
            if (strcmp(prop_type_str, "bool") == 0) {
                value.type = TiledPropertyType::Bool;
                value.value_bool = prop_value.as_bool();
            }
            else if (strcmp(prop_type_str, "color") == 0) {
                value.type = TiledPropertyType::Color;
                value.value_color = Colors::from_hex(prop_value.value());
            }
            else if (strcmp(prop_type_str, "float") == 0) {
                value.type = TiledPropertyType::Float;
                value.value_float = prop_value.as_float();
            }
            else if (strcmp(prop_type_str, "file") == 0) {
                value.type = TiledPropertyType::File;
                value.value_string = prop_value.as_string();
            }
            else if (strcmp(prop_type_str, "int") == 0) {
                value.type = TiledPropertyType::Int;
                value.value_int = prop_value.as_int();
            }
            else if (strcmp(prop_type_str, "object") == 0) {
                value.type = TiledPropertyType::Object;
                value.value_int = prop_value.as_int();
            }
            else if (strcmp(prop_type_str, "string") == 0) {
                value.type = TiledPropertyType::String;
                value.value_string = prop_value.as_string();
            }
            obj.properties.insert({prop_name, value});
        }
    }
    return obj;
}

TiledObjectTemplate* Tilemap::load_template_object(const std::string& filepath) {
    auto it = templates.find(std::string(filepath));
    if (it != templates.end()) {
        return &it->second;
    }
    else {
        pugi::xml_document doc;
        std::vector<char> buf = load_file_to_buffer(filepath);
        if (buf.empty()) return nullptr;
        pugi::xml_parse_result result = doc.load_buffer(buf.data(), buf.size());
        if (!result) {
            log_error("Error while parsing Tiled XML file.");
            return nullptr;
        }
        auto tmpl = parse_template_object(doc.child("template"));
        return &templates.insert({filepath, tmpl}).first->second;
    }
}

TiledObjectTemplate Tilemap::parse_template_object(const pugi::xml_node &node) {
    TiledObjectTemplate tmpl;
    tmpl.obj = parse_object(node.child("object"));
    auto node_tileset = node.child("tileset");
    tmpl.tileset_firstgid = node_tileset.attribute("firstgid").as_int();
    tmpl.tileset_source = strip_relative_path(node_tileset.attribute("source").value());
    return tmpl;
}

TiledLayer Tilemap::parse_layer(const pugi::xml_node& node) {
    TiledLayer layer;
    layer.id = node.attribute("id").as_int();
    layer.name = node.attribute("name").value();
    if (strcmp(node.name(), "layer") == 0) {
        layer.type = TiledLayer::Type::Tile;
        layer.width = node.attribute("width").as_int();
        layer.height = node.attribute("height").as_int();
        auto node_data = node.child("data");
        std::string_view csv = node_data.text().get();
        size_t last = 0, next = 0;
        layer.data.resize(layer.width*layer.height);
        int counter = 0;
        while (true) {
            next = csv.find(',', last);
            if (csv[last] == '\n') last++;
            std::string_view token = csv.substr(last, next-last);
            int value = strbuf_to_int32(token.data(), token.size());
            layer.data[counter] = value;
            counter++;
            if (next == std::string::npos || counter == layer.data.size()) {
                break;
            }
            last = next + 1;
        }
    }
    else if (strcmp(node.name(), "objectgroup") == 0) {
        layer.type = TiledLayer::Type::ObjectGroup;
        for (auto node_obj : node.children("object")) {
            TiledObject obj = parse_object(node_obj);
            layer.object_names.insert({obj.name, layer.objects.size()});
            layer.objects.push_back(obj);
        }
    }
    else {
        log_fatal("Cannot load layer %s from TMX file!", layer.name.c_str());
        std::abort();
    }
    return layer;
}

Ref<Tilemap> Tilemap::load(const char* filename) {
    pugi::xml_document doc;
    {
        std::vector<char> buf = load_file_to_buffer(filename);
        if (buf.empty()) return {};
        pugi::xml_parse_result result = doc.load_buffer(buf.data(), buf.size());
        if (!result) {
            log_error("Error while parsing Tiled XML file {}.", filename);
            return {};
        }
    }

    auto res = Engine::instance().get_resources();
    auto tilemap_ref = res->new_item<Tilemap>();
    Tilemap& tilemap = *tilemap_ref.get();

    auto node_map = doc.child("map");
    tilemap.version = node_map.attribute("version").value();
    tilemap.tiledversion = node_map.attribute("tiledversion").value();
    tilemap.width = node_map.attribute("width").as_int();
    tilemap.height = node_map.attribute("height").as_int();
    tilemap.tilewidth = node_map.attribute("tilewidth").as_int();
    tilemap.tileheight = node_map.attribute("tileheight").as_int();
    tilemap.infinite = node_map.attribute("infinite").as_int();

    for (auto node_tileset : node_map.children("tileset")) {
        TilesetRef tileset_ref;
        tileset_ref.firstgid = node_tileset.attribute("firstgid").as_int();
        auto tileset_file = node_tileset.attribute("source").value();
        tileset_ref.ref = Tileset::load(tileset_file);
        tilemap.tilesets.push_back(tileset_ref);
    }
    for (auto node_layer_group : node_map.children("group")) {
        TiledLayerGroup group;
        group.id = node_layer_group.attribute("id").as_int();
        group.name = node_layer_group.attribute("name").as_string();
        for (auto node_layer : node_layer_group.children()) {
            group.layers.push_back(std::move(tilemap.parse_layer(node_layer)));
        }
        tilemap.layer_groups.push_back(std::move(group));
    }
    TiledLayerGroup default_group;
    default_group.id = 0;
    default_group.name = "default";
    for (auto node_layer : node_map.children("layer")) {
        default_group.layers.push_back(std::move(tilemap.parse_layer(node_layer)));
    }
    tilemap.layer_groups.push_back(std::move(default_group));

    return tilemap_ref;
}

void Tilemap::insert_to_scene() {
    auto& engine = Engine::instance();
    auto vm = engine.get_vm();
    auto res = engine.get_resources();
    auto col_mgr = engine.get_collision_manager();

    for (auto& layer_group : layer_groups) {
        uint16_t layer_group_id;
        if (layer_group.name == "Background") {
            layer_group_id = Layers::Background;
        }
        else if (layer_group.name == "Sprite") {
            layer_group_id = Layers::Sprite;
        }
        else if (layer_group.name == "Foreground") {
            layer_group_id = Layers::Foreground;
        }
        else if (layer_group.name == "UI") {
            layer_group_id = Layers::UI;
        }
        else {
            // UI layer by default
            layer_group_id = Layers::UI;
        }

        for (int layer_id = 0; layer_id < layer_group.layers.size(); layer_id++) {
            auto& layer = layer_group.layers[layer_id];
            if (layer.type == TiledLayer::Type::Tile) {
                if (layer_group_id == Layers::Sprite && layer.name == "Player") {
                    player_layer_idx = layer_id;
                }
                for (auto& tileset_ref : tilesets) {
                    auto& tileset = *tileset_ref.ref.get();
                    if (tileset.type != Tileset::Type::Image) continue;
                    for (int y = 0; y < height; y++) {
                        for (int x = 0; x < width; x++) {
                            int idx = y * width + x;
                            if (idx < layer.data.size() &&
                                layer.data[idx] >= tileset_ref.firstgid &&
                                layer.data[idx] < (tileset_ref.firstgid + tileset.tilecount)) {

                                int id = layer.data[idx] - tileset_ref.firstgid;
                                ivec2 tileset_size = ivec2(tileset.tilewidth, tileset.tileheight);
                                ivec2 tileset_count = ivec2(tileset.columns, tileset.tilecount / tileset.columns);
                                ivec2 index_pos = vec2(id % tileset_count.x, id / tileset_count.x);

                                Sprite::Options opt;
                                opt.srcrect.pos = index_pos * tileset_size;
                                opt.srcrect.size = tileset_size;
                                opt.pos = vec2(x * tileset.tilewidth, y * tileset.tileheight);
                                opt.scale = vec2(1, 1);
                                opt.origin = vec2(0, 0);
                                opt.rot = 0;
                                opt.color = rgba(0xffffffff);
                                opt.layer = layer_group_id;
                                opt.z_index = layer_id;
                                opt.tex_ref = tileset.image_tex_ref;
                                auto sprite_ref = res->new_item<Sprite>(opt);
                                auto sprite = sprite_ref.get();

                                auto& tsobj = tileset.objects[id];
                                for (auto col_data : tsobj.colliders) {
                                    auto col_ref = col_mgr->create_collider(col_data);
                                    sprite->add_child(col_ref.cast_unsafe<Node>());
                                }
                            }
                        }
                    }
                }
            }
            else if (layer.type == TiledLayer::Type::ObjectGroup) {
                for (auto& tobj : layer.objects) {
                    if (tobj.gid == 0) continue; // Skip if object is not from object collection

                    // TODO: fix O(n) lookup
                    int tileset_idx;
                    for (tileset_idx = tilesets.size()-1; tileset_idx >= 0; tileset_idx--) {
                        if (tilesets[tileset_idx].firstgid <= tobj.gid) break;
                    }
                    auto tileset_ref = tilesets[tileset_idx];
                    auto tileset = tileset_ref.ref.get();
                    int obj_id = tobj.gid - tileset_ref.firstgid;
                    auto& tsobj = tileset->objects[obj_id];

                    Sprite::Options opt;
                    opt.tex_ref = Texture::from_image_file(tsobj.source);
                    opt.srcrect = irect(0, 0, tsobj.width, tsobj.height);
                    opt.pos = ivec2(tobj.x, tobj.y - tsobj.height);
                    opt.scale = vec2(1, 1);
                    opt.origin = vec2(0, 0);
                    opt.rot = tobj.rotation;
                    opt.color = rgba(0xffffffff);
                    opt.layer = Layers::Sprite;
                    opt.z_index = layer_id;

                    Ref<Node> node_ref;
                    if (tobj.script) {
                        // If script is attached, then create a ScriptableNode
                        auto script = tobj.script.get();
                        auto cls = sq::Class(script->exports);
                        sq::Table args = opt.to_sqtable();
                        sq::Instance inst = vm->new_instance(cls, args);
                        if (!vm->try_get_cppref(inst, node_ref)) {
                            log_error("Error while creating Squirrel object from Tiled: script does not extend Node");
                        }
                    }
                    else {
                        // Else, just create a Sprite
                        auto sprite_ref = res->new_item<Sprite>(opt);
                        node_ref = sprite_ref.cast_unsafe<Node>();
                    }

                    // Create colliders as child nodes
                    for (auto col_data : tsobj.colliders) {
                        auto col_ref = col_mgr->create_collider(col_data);
                        node_ref.get()->add_child(col_ref.cast_unsafe<Node>());
                        if (node_ref.inherits_type<KinematicBody>()) {
                            auto body_ref = node_ref.cast_unsafe<KinematicBody>();
                            body_ref.get()->add_collider(col_ref);
                        }
                    }
                }
            }
        }
    }
}
