//
// Created by lasagnaphil on 10/17/2021.
//

#ifndef THESYSTEM_TEXT_H
#define THESYSTEM_TEXT_H

#include "font.h"
#include "core/color.h"
#include "core/reflect.h"
#include "squirrel/object.h"
#include "render/node.h"
#include <string>

class Engine;

CLASS(Resource) Text : public Node {
public:
    CLASS(OptionFor=Text) Options : public Node::Options {
    public:
        Ref<Font> font_ref;
        std::string contents;
        rgba color;

        Options() = default;
        Options(sq::Table table);
    };

    Text() = default;
    Text(const Options& opt);

    FUNCTION(getter)
    std::string get_contents() { return contents; }
    FUNCTION(setter)
    void set_contents(const std::string& contents) {
        this->contents = contents;
        is_dirty = true;
    }

    FUNCTION(getter)
    Ref<Font> get_font() { return font_ref; }
    FUNCTION(setter)
    void set_font(Ref<Font> font_ref) {
        this->font_ref = font_ref;
        is_dirty = true;
    }

    FUNCTION(getter)
    uint16_t get_layer() { return layer; }
    FUNCTION(setter)
    void set_layer(uint16_t layer) {
        this->layer = layer;
        is_dirty = true;
    }

    void update(float dt);

private:
    std::string contents;
    rgba color;

    bool is_dirty = true;

    Ref<Font> font_ref;
    std::vector<Ref<Sprite>> sprites;
};

#endif //THESYSTEM_TEXT_H
