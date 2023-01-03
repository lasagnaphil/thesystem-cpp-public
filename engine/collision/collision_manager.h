#pragma once

#include <parallel_hashmap/phmap.h>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "collider.h"

#include "core/types.h"
#include "core/rect.h"
#include "core/reflect.h"
#include "render/node.h"
#include "resource_pool.h"

#include <vector>

// A simple spatial hash structure that checks collision between colliders.

struct SpatialHashEntry {
    std::vector<Ref<Collider>> colliders;
};

CLASS() CollisionManager {
public:
    friend class Collider;

    CollisionManager(float cell_size = 32) : cell_size(cell_size) {}

    Ref<Collider> create_collider(const Collider::Options& opt);

    void update();

    void update_partial(const std::vector<Ref<Collider>>& colliders,
                        std::vector<ContactEvent>& out_contact_events);

    void debug_render();

private:
    void add_collider(Ref<Collider> col_ref);
    void remove_collider(Ref<Collider> col_ref);
    void update_collider(Ref<Collider> col_ref);

    float cell_size;
    phmap::flat_hash_map<ivec2, SpatialHashEntry> spatial_hash;

    std::vector<ContactEvent> contact_events;
};
