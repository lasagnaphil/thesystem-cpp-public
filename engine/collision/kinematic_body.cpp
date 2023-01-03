#include "kinematic_body.h"

#include "collision/collision_manager.h"
#include "squirrel/vm.h"

KinematicBody::Options::Options(sq::Table table) {
    auto vm = Engine::instance().get_vm();
    margin = vm->get_or_default<float>(table, "margin", margin);
}

void KinematicBody::move_and_collide(vec2 dx) {
    log_error("KinematicBody::move_and_collide() is unimplemented!");

    auto old_pos = get_pos();
    translate(dx);

    auto col_mgr = Engine::instance().get_collision_manager();
    std::vector<ContactEvent> contact_events;
    col_mgr->update_partial(colliders, contact_events);

    if (!contact_events.empty()) {
        set_pos(old_pos);
        // TODO: Need to do CCD?
    }
}

void KinematicBody::move_and_slide(vec2 dx) {
    auto old_pos = get_pos();
    translate(dx);

    auto col_mgr = Engine::instance().get_collision_manager();
    std::vector<ContactEvent> contact_events;

    while (true) {
        contact_events.clear();
        col_mgr->update_partial(colliders, contact_events);
        if (contact_events.empty()) break;

        set_pos(old_pos);
        int max_depth = INT32_MIN;
        int max_depth_contact_idx = -1;
        for (int i = 0; i < contact_events.size(); i++) {
            if (contact_events[i].depth > max_depth) {
                max_depth = contact_events[i].depth;
                max_depth_contact_idx = i;
            }
        }
        auto& ev = contact_events[max_depth_contact_idx];
        dx -= ev.depth * ev.normal;
        translate(dx);
    }
}

void KinematicBody::add_collider(Ref<Collider> col_ref) {
    colliders.push_back(col_ref);
}

void KinematicBody::add_child_colliders() {
    for (auto node_ref : children) {
        if (node_ref.inherits_type<Collider>()) {
            colliders.push_back(node_ref.cast_unsafe<Collider>());
        }
    }
}

void KinematicBody::add_child_colliders_recursive() {
    std::vector<Ref<Node>> stack;
    stack.push_back(get_self());
    while (!stack.empty()) {
        Ref<Node> node_ref = stack.at(stack.size()-1);
        stack.pop_back();
        auto children = node_ref.get()->get_children();
        for (auto it = children.rbegin(); it != children.rend(); it++) {
            if (it->inherits_type<Collider>()) {
                colliders.push_back(it->cast_unsafe<Collider>());
            }
            stack.push_back(*it);
        }
    }
}
