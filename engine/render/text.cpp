//
// Created by lasagnaphil on 10/17/2021.
//

#include "text.h"
#include "engine.h"
#include "sprite.h"
#include "squirrel/vm.h"
#include "squirrel/utils.h"

Text::Options::Options(sq::Table table) : Node::Options(table) {
    auto& engine = Engine::instance();
    auto& vm = *engine.get_vm();
    font_ref = vm.get<Ref<Font>>(table, "font");
    contents = vm.get_or_default<std::string>(table, "contents", "");
    color = vm.get_or_default<rgba>(table, "color", rgba(0xffffffff));
}

Text::Text(const Options &opt) : Node(opt) {
    font_ref = opt.font_ref,
    contents = opt.contents;
    color = opt.color;
}

void Text::update(float dt) {
    if (!is_dirty) return;

    auto res = Engine::instance().get_resources();
    for (auto sprite : sprites) {
        remove_child(sprite.cast_unsafe<Node>());
        res->get_pool<Sprite>().release(sprite);
    }
    sprites.clear();

    // TODO: implement Unicode
    const auto& font = *font_ref.get();
    int space_width = font.chars[font.char_map.at(' ')].xadvance;
    int space_height = font.chars[font.char_map.at('l')].height;
    int counter = 0;
    float cx = 0, cy = 0;
    sprites.reserve(contents.size());
    for (char c : contents) {
        if (c == ' ') {
            cx += space_width;
            continue;
        }
        if (c == '\t') {
            cx += 4 * space_width;
            continue;
        }
        else if (c == '\n') {
            cx = 0;
            cy += space_height + 4;
            continue;
        }
        auto sprite_ref = res->new_item<Sprite>();
        add_child(sprite_ref.cast_unsafe<Node>());
        sprites.push_back(sprite_ref);
        uint32_t char_id = font.char_map.at((uint32_t)c);
        auto& ch = font.chars[char_id];
        auto sprite = sprite_ref.get();
        sprite->srcrect.pos = {ch.x, ch.y};
        sprite->srcrect.size = {ch.width, ch.height};
        sprite->pos = {cx + ch.xoffset, cy + ch.yoffset};
        sprite->scale = {1, 1};
        sprite->origin = {0, 0};
        sprite->rot = 0;
        sprite->color = color;
        sprite->layer = layer;
        sprite->z_index = z_index;
        sprite->update_enabled = update_enabled;
        sprite->render_enabled = render_enabled;
        sprite->tex_ref = font.pages[ch.page];
        sprite->_update_xform();
        cx += ch.xadvance;
        counter++;
    }

    is_dirty = false;
}
