#include "node.h"

#include "engine.h"
#include "resources.h"
#include "squirrel/vm.h"

#include <Tracy.hpp>

Node::Options::Options(sq::Table table) {
    ZoneScoped
    auto& engine = Engine::instance();
    auto& vm = *engine.get_vm();
    pos = vm.get_or_default<vec2>(table, "pos", vec2(0, 0));
    scale = vm.get_or_default<vec2>(table, "scale", vec2(1, 1));
    rot = vm.get_or_default<float>(table, "rot", 0);
    layer = vm.get_or_default<uint16_t>(table, "layer", Layers::Sprite);
    z_index = vm.get_or_default<uint16_t>(table, "z_index", 0);
    bool update_render_enabled;
    if (vm.try_get<bool>(table, "enabled", update_render_enabled)) {
        update_enabled = render_enabled = update_render_enabled;
    }
    update_enabled = vm.get_or_default<bool>(table, "update_enabled", true);
    render_enabled = vm.get_or_default<bool>(table, "render_enabled", true);
}

sq::Table Node::Options::to_sqtable() {
    auto vm = Engine::instance().get_vm();
    sq::Table args = vm->new_table();
    vm->setslot(args, "pos", pos);
    vm->setslot(args, "scale", scale);
    vm->setslot(args, "rot", rot);
    vm->setslot(args, "layer", layer);
    vm->setslot(args, "z_index", z_index);
    vm->setslot(args, "update_enabled", update_enabled);
    vm->setslot(args, "render_enabled", render_enabled);
    return args;
}

Node::Node(const Node::Options& opt) {
    pos = opt.pos;
    scale = opt.scale;
    rot = opt.rot;
    layer = opt.layer;
    z_index = opt.z_index;
    update_enabled = opt.update_enabled;
    render_enabled = opt.render_enabled;
    _update_xform();
}

Ref<Node> Node::get_self() const {
    // TODO: this is wrong because it isn't guaranteed that this object will reside in Node pool
    auto node = StableResourcePool<Node>::get_node_from_item(this);
    return Ref<Node>(this, node->meta.generation);
}

void Node::set_parent(Ref<Node> parent_ref) {
    auto self = get_self();
    assert(self.check());
    parent_ref.get()->children.push_back(self);
    this->parent = parent_ref;
    this->_global_xform_dirty = true;
}

void Node::add_child(Ref<Node> child_ref) {
    auto self = get_self();
    assert(self.check());
    auto child = child_ref.get();
    child->parent = self;
    child->_global_xform_dirty = true;
    children.push_back(child_ref);
}

void Node::remove_child(Ref<Node> child_ref) {
    auto it = std::find(children.begin(), children.end(), child_ref);
    if (it != children.end()) {
        children.erase(it);
        auto child = child_ref.get();
        child->parent = {};
        child->_global_xform_dirty = true;
    }
    else {
        log_warn("Trying to remove invalid child!");
    }
}

void Node::set_update_enable(bool p_enable) {
    update_enabled = p_enable;
    for (auto child_ref : children) {
        child_ref.get()->update_enabled = p_enable;
    }
}

void Node::set_render_enable(bool p_enable) {
    render_enabled = p_enable;
    for (auto child_ref : children) {
        child_ref.get()->render_enabled = p_enable;
    }
}

void Node::_notify_transform() {
    if (_global_xform_dirty) return;

    _global_xform_dirty = true;
    for (auto child_ref : children) {
        child_ref.get()->_notify_transform();
    }
}

void Node::_update_xform_values() {
    if (!_xform_dirty) return;

    pos = transform[2];
    rot = transform.get_rotation();
    scale = transform.get_scale();
    skew = transform.get_skew();
    _xform_dirty = false;
}

void Node::_update_xform() {
    transform.set_rotation_scale_and_skew(rot, scale, skew);
    transform[2] = pos;
    _notify_transform();
}

void Node::_update_global_xform() {
    if (!_global_xform_dirty) return;

    if (parent) {
        global_transform = parent.get()->get_global_trans() * transform;
    }
    else {
        global_transform = transform;
    }
    _global_xform_dirty = false;
}
