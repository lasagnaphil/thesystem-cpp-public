//
// Created by lasagnaphil on 10/24/2021.
//

#include "collision_manager.h"

#include "engine.h"
#include "resources.h"

#include "sokol_gp.h"

#include <glm/gtx/norm.hpp>

bool aabb_aabb_collision(Collider* col1, Collider* col2, ContactEvent& ev) {
    AABB isect;
    isect.min = glm::max(col1->bounds.min, col2->bounds.min);
    isect.max = glm::min(col1->bounds.max, col2->bounds.max);
    if (isect.min.x < isect.max.x && isect.min.y < isect.max.y) {
        ev.point = isect.center();
        auto col_size = isect.size();
        auto p1 = col1->bounds.center();
        auto p2 = col2->bounds.center();
        if (col_size.x <= col_size.y) {
            ev.normal = vec2(p1.x < ev.point.x? 1 : -1, 0);
            ev.depth = col_size.x;
        }
        else {
            ev.normal = vec2(0, p1.y < ev.point.y? 1 : -1);
            ev.depth = col_size.y;
        }
        return true;
    }
    else {
        return false;
    }
}

bool aabb_obb_collision(Collider* col1, Collider* col2, ContactEvent& ev) {
    return false;
}

bool aabb_circle_collision(Collider* col1, Collider* col2, ContactEvent& ev) {
    return false;
}

bool obb_obb_collision(Collider* col1, Collider* col2, ContactEvent& ev) {
    return false;
}

bool obb_circle_collision(Collider* col1, Collider* col2, ContactEvent& ev) {
    return false;
}

bool circle_circle_collision(Collider* col1, Collider* col2, ContactEvent& ev) {
    auto& t1 = col1->_get_global_trans_raw();
    auto& t2 = col2->_get_global_trans_raw();
    vec2 p1 = t1.get_origin();
    vec2 p2 = t2.get_origin();
    vec2 p = p2 - p1;
    float r_tot = col1->circle.radius + col2->circle.radius;
    float p_len_sq = glm::length2(p);
    if (p_len_sq <= r_tot * r_tot) {
        float p_len = glm::sqrt(p_len_sq);
        ev.point = 0.5f * (p1 + p2);
        ev.normal = p / p_len;
        ev.depth = p_len - r_tot;
        return true;
    }
    else {
        return false;
    }
}

bool (*dispatch_func_matrix[3][3])(Collider*, Collider*, ContactEvent&) = {
        {aabb_aabb_collision, aabb_obb_collision, aabb_circle_collision},
        {nullptr,             obb_obb_collision,  obb_circle_collision},
        {nullptr,             nullptr,            circle_circle_collision}
};

Ref<Collider> CollisionManager::create_collider(const Collider::Options& opt) {
    auto res = Engine::instance().get_resources();
    auto col_ref = res->new_item<Collider>(opt);
    add_collider(col_ref);
    return col_ref;
}

void CollisionManager::update() {
    auto res = Engine::instance().get_resources();

    // Update transforms and AABB bounds
    res->foreach_ref<Collider>([this](Ref<Collider> col_ref, Collider& col) {
        col.contact_events.clear();
        update_collider(col_ref);
    });

    // Find contact events
    contact_events.clear();
    for (auto& [ipos, entry] : spatial_hash) {
        int n_cols = entry.colliders.size();
        for (int i = 0; i < n_cols; i++) {
            auto col_ref = entry.colliders[i];
            auto col = col_ref.get();
            for (int j = i+1; j < n_cols; j++) {
                auto col1 = col;
                auto col1_ref = col_ref;
                auto col2_ref = entry.colliders[j];
                auto col2 = col2_ref.get();
                if (!col1->bounds.collides_with(col2->bounds)) {
                    continue;
                }
                if ((int)col1->type > (int)col2->type) {
                    std::swap(col1, col2);
                    std::swap(col1_ref, col2_ref);
                }
                auto fun = dispatch_func_matrix[(int)col1->type][(int)col2->type];
                ContactEvent ev;
                if (fun(col1, col2, ev)) {
                    ev.col1 = col1_ref;
                    ev.col2 = col2_ref;
                    contact_events.push_back(ev);

                    col1->contact_events.push_back(ev);
                    col2->contact_events.push_back(ev);
                }
            }
        }
    }
}

void CollisionManager::update_partial(
        const std::vector<Ref<Collider>>& colliders,
        std::vector<ContactEvent>& out_contact_events) {

    for (auto col_ref : colliders) {
        update_collider(col_ref);
    }
    for (auto col_ref : colliders) {
        auto col = col_ref.get();
        ivec2 imin = col->bounds.min / cell_size;
        ivec2 imax = col->bounds.max / cell_size;
        for (int x = imin.x; x <= imax.x; x++) {
            for (int y = imin.y; y <= imax.y; y++) {
                auto& entry = spatial_hash.at(ivec2(x, y));
                auto col1 = col;
                auto col1_ref = col_ref;
                for (auto col2_ref : entry.colliders) {
                    if (col1_ref == col2_ref) continue;

                    auto col2 = col2_ref.get();
                    if (!col1->bounds.collides_with(col2->bounds)) {
                        continue;
                    }
                    if ((int)col1->type > (int)col2->type) {
                        std::swap(col1, col2);
                        std::swap(col1_ref, col2_ref);
                    }
                    auto fun = dispatch_func_matrix[(int)col1->type][(int)col2->type];
                    ContactEvent ev;
                    if (fun(col1, col2, ev)) {
                        // Always make col1 be the requested collider reference
                        if (col1 == col) {
                            ev.col1 = col1_ref;
                            ev.col2 = col2_ref;
                        }
                        else {
                            ev.col1 = col2_ref;
                            ev.col2 = col1_ref;
                            ev.normal = -ev.normal; // flip normals
                        }
                        out_contact_events.push_back(ev);
                    }
                }
            }
        }
    }
}

void CollisionManager::add_collider(Ref<Collider> col_ref) {
    auto col = col_ref.get();
    ivec2 imin = col->bounds.min / cell_size;
    ivec2 imax = col->bounds.max / cell_size;
    for (int x = imin.x; x <= imax.x; x++) {
        for (int y = imin.y; y <= imax.y; y++) {
            auto it = spatial_hash.find(ivec2(x, y));
            if (it != spatial_hash.end()) {
                it->second.colliders.push_back(col_ref);
            }
            else {
                SpatialHashEntry entry;
                entry.colliders.push_back(col_ref);
                spatial_hash.insert({ivec2(x, y), std::move(entry)});
            }
        }
    }
}

void CollisionManager::remove_collider(Ref<Collider> col_ref) {
    auto col = col_ref.get();
    ivec2 imin = col->bounds.min / cell_size;
    ivec2 imax = col->bounds.max / cell_size;
    for (int x = imin.x; x <= imax.x; x++) {
        for (int y = imin.y; y <= imax.y; y++) {
            auto it = spatial_hash.find(ivec2(x, y));
            if (it != spatial_hash.end()) {
                auto& colliders = it->second.colliders;
                colliders.erase(std::find(colliders.begin(), colliders.end(), col_ref));
            }
        }
    }
}

void CollisionManager::update_collider(Ref<Collider> col_ref) {
    auto col = col_ref.get();
    AABB new_bounds;
    auto t = col->get_global_trans();
    auto center = t.get_origin();
    switch (col->type) {
        case ColliderType::AABB: {
            new_bounds.min = center - col->aabb.extents;
            new_bounds.max = center + col->aabb.extents;
        } break;
        case ColliderType::OBB: {
            auto u = t.basis_xform(vec2(col->obb.extents.x, 0));
            auto v = t.basis_xform(vec2(0, col->obb.extents.y));
            vec2 max_extents = glm::max(u, v);
            new_bounds.min = center - max_extents;
            new_bounds.max = center + max_extents;
        } break;
        case ColliderType::Circle: {
            new_bounds.min = center - col->circle.radius;
            new_bounds.max = center + col->circle.radius;
        } break;
    }
    ivec2 imin = col->bounds.min / cell_size;
    ivec2 imax = col->bounds.max / cell_size;
    ivec2 new_imin = new_bounds.min / cell_size;
    ivec2 new_imax = new_bounds.max / cell_size;

    // Remove old entries
    for (int x = imin.x; x <= imax.x; x++) {
        for (int y = imin.y; y <= imax.y; y++) {
            if (x < new_imin.x || x > new_imax.x || y < new_imin.y || y > new_imax.y) {
                auto& entry = spatial_hash.at(ivec2(x, y));
                auto& colliders = entry.colliders;
                colliders.erase(std::find(colliders.begin(), colliders.end(), col_ref));
            }
        }
    }

    // Add new entries
    for (int x = new_imin.x; x <= new_imax.x; x++) {
        for (int y = new_imin.y; y <= new_imax.y; y++) {
            if (x < imin.x || x > imax.x || y < imin.y || y > imax.y) {
                auto it = spatial_hash.find(ivec2(x, y));
                if (it != spatial_hash.end()) {
                    it->second.colliders.push_back(col_ref);
                }
                else {
                    SpatialHashEntry entry;
                    entry.colliders.push_back(col_ref);
                    spatial_hash.insert({ivec2(x, y), std::move(entry)});
                }
            }
        }
    }

    // Update bounds
    col->bounds = new_bounds;
}

void CollisionManager::debug_render() {
    auto res = Engine::instance().get_resources();
    sgp_set_color(1, 1, 1, 1);
    for (auto& [ipos, entry] : spatial_hash) {
        if (entry.colliders.empty()) continue;
        sgp_point p[4];
        p[0] = {(ipos.x + 0)*cell_size, (ipos.y + 0)*cell_size};
        p[1] = {(ipos.x + 0)*cell_size, (ipos.y + 1)*cell_size};
        p[2] = {(ipos.x + 1)*cell_size, (ipos.y + 1)*cell_size};
        p[3] = {(ipos.x + 1)*cell_size, (ipos.y + 0)*cell_size};
        sgp_line l[4];
        l[0] = {p[0], p[1]};
        l[1] = {p[1], p[2]};
        l[2] = {p[2], p[3]};
        l[3] = {p[3], p[0]};
        sgp_draw_lines(l, 4);
    }
    sgp_set_color(0, 1, 0, 1);
    res->foreach<Collider>([](Collider& col) {
        auto t = col.get_global_trans();
        auto pos = t.get_origin();
        auto vec2_conv = [](vec2 p) -> sgp_point { return {p.x, p.y}; };
        {
            sgp_point p[4];
            p[0] = {col.bounds.min.x, col.bounds.min.y};
            p[1] = {col.bounds.min.x, col.bounds.max.y};
            p[2] = {col.bounds.max.x, col.bounds.max.y};
            p[3] = {col.bounds.max.x, col.bounds.min.y};
            sgp_line l[4];
            l[0] = {p[0], p[1]};
            l[1] = {p[1], p[2]};
            l[2] = {p[2], p[3]};
            l[3] = {p[3], p[0]};
            sgp_draw_lines(l, 4);
        }
        switch (col.type) {
            case ColliderType::OBB: {
                vec2 u = t.basis_xform(vec2(col.obb.extents.x, 0));
                vec2 v = t.basis_xform(vec2(0, col.obb.extents.y));
                sgp_point p[4];
                p[0] = vec2_conv(pos - u - v);
                p[1] = vec2_conv(pos - u + v);
                p[2] = vec2_conv(pos + u + v);
                p[3] = vec2_conv(pos + u - v);
                sgp_line l[4];
                l[0] = {p[0], p[1]};
                l[1] = {p[1], p[2]};
                l[2] = {p[2], p[3]};
                l[3] = {p[3], p[0]};
                sgp_draw_lines(l, 5);
            } break;
            case ColliderType::Circle: {
                constexpr int subdivs = 20;
                const float two_pi = glm::two_pi<float>();
                sgp_point p[subdivs+1];
                for (int i = 0; i < subdivs; i++) {
                    float theta = two_pi/subdivs;
                    p[i] = vec2_conv(pos + col.circle.radius * vec2(glm::cos(theta), glm::sin(theta)));
                }
                p[subdivs] = p[0];
                sgp_line l[subdivs];
                for (int i = 0; i < subdivs; i++) {
                    l[i] = {p[i], p[i+1]};
                }
                sgp_draw_lines(l, subdivs);
            } break;
        }
    });
    sgp_set_color(1, 0, 0, 1);
    for (auto& ev : contact_events) {
        sgp_draw_point(ev.point.x, ev.point.y);
        sgp_draw_line(ev.point.x, ev.point.y, ev.point.x + 10 * ev.normal.x, ev.point.y + 10 * ev.normal.y);
        sgp_draw_line(ev.point.x, ev.point.y, ev.point.x - 10 * ev.normal.x, ev.point.y - 10 * ev.normal.y);
    }
    sgp_reset_color();
}
