//
// Created by lasagnaphil on 2021-08-01.
//

#include "sprite.h"

#include "engine.h"
#include "squirrel/vm.h"

#include <Tracy.hpp>

Sprite::Options::Options(sq::Table table) : Node::Options(table) {
    ZoneScoped
    auto& engine = Engine::instance();
    auto& vm = *engine.get_vm();
    auto& res = *engine.get_resources();
    tex_ref = vm.get_or_default<Ref<Texture>>(table, "tex", Ref<Texture>());
    if (tex_ref) {
        auto tex_size = tex_ref.get()->get_size();
        srcrect = vm.get_or_default<irect>(table, "srcrect", irect({0, 0}, tex_size));
    }
    else {
        srcrect = irect(0, 0, 0, 0);
    }
    origin = vm.get_or_default<vec2>(table, "origin", vec2(0, 0));
    color = rgba(vm.get_or_default<uint32_t>(table, "color", 0xffffffff));
}

sq::Table Sprite::Options::to_sqtable() {
    auto vm = Engine::instance().get_vm();
    sq::Table args = Node::Options::to_sqtable();
    vm->setslot(args, "tex", tex_ref);
    vm->setslot(args, "srcrect", srcrect);
    vm->setslot(args, "origin", origin);
    vm->setslot(args, "color", color);
    return args;
}

Sprite::Sprite(const Sprite::Options &opt) : Node(opt) {
    tex_ref = opt.tex_ref;
    srcrect = opt.srcrect;
    origin = opt.origin;
    color = opt.color;
}
