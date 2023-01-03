#include "collider.h"
#include "collision_manager.h"

Collider::Options::Options(sq::Table table) {
}

sq::Table Collider::Options::to_sqtable() {
    return sq::Table();
}

Collider::Collider(const Options& opt) {
    type = opt.type;
    switch(type) {
        case ColliderType::Circle: {
            circle.radius = opt.circle.radius;
        } break;
        case ColliderType::AABB: {
            aabb.extents = opt.aabb.extents;
        } break;
        case ColliderType::OBB: {
            obb.extents = opt.obb.extents;
        } break;
    }

    set_pos(opt.pos);
    set_rot(opt.rot);
}
