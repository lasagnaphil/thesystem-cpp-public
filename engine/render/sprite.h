//
// Created by lasagnaphil on 2021-08-01.
//

#ifndef THESYSTEM_SPRITE_H
#define THESYSTEM_SPRITE_H

#include <vector>

#include "texture.h"
#include "core/rect.h"
#include "core/color.h"
#include "core/reflect.h"
#include "squirrel/object.h"
#include "render/node.h"

CLASS(Resource) Sprite : public Node {
public:
    CLASS(OptionFor=Sprite) Options : public Node::Options {
    public:
        Ref<Texture> tex_ref;
        irect srcrect;
        vec2 origin;
        rgba color;

        Options() = default;
        Options(sq::Table table);

        sq::Table to_sqtable();
    };

    friend class SpriteRenderer;
    friend class Text;
    friend class Tilemap;
    friend class Animation;

    PROPERTY()
    irect srcrect;
    PROPERTY()
    vec2 origin;
    PROPERTY()
    rgba color;

    PROPERTY()
    Ref<Texture> tex_ref;

    Sprite() = default;
    Sprite(const Options& opt);

    FUNCTION(getter)
    float get_width() const { return srcrect.size.x * scale.x; }
    FUNCTION(getter)
    float get_height() const { return srcrect.size.y * scale.y; }

    FUNCTION(setter)
    void set_srcrect(irect _srcrect) { srcrect = _srcrect; }
    FUNCTION(setter)
    void set_origin(vec2 _origin) { origin = _origin; }
    FUNCTION(setter)
    void set_color(rgba _color) { color = _color; }
};

#endif //THESYSTEM_SPRITE_H
