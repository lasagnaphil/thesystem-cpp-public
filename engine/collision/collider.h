#pragma once

#include "core/types.h"
#include "core/reflect.h"
#include "core/rect.h"
#include "resource_pool.h"
#include "render/node.h"

struct AABB {
    vec2 min;
    vec2 max;

    AABB() = default;
    AABB(rect rect) : min(rect.pos - 0.5f*rect.size), max(rect.pos + 0.5f*rect.size) {}

    inline vec2 center() const { return 0.5f * (min + max); }
    inline vec2 size() const { return max - min; }
    inline float volume() const { return (max.x - min.x) * (max.y - min.y); }

    bool collides_with(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y);
    }
};

struct ContactEvent {
    Ref<Collider> col1, col2;
    vec2 point;
    vec2 normal;
    float depth;
};

enum class ColliderType {
    AABB, OBB, Circle,
};

CLASS(Resource, Node) Collider : public Node {
public:
    CLASS(OptionFor=Collider) Options {
    public:
        ColliderType type;
        vec2 pos = {0, 0};
        float rot = 0;
        union {
            struct {
                vec2 extents;
            } aabb;
            struct {
                vec2 extents;
            } obb;
            struct {
                float radius;
            } circle;
        };

        Options() = default;
        Options(sq::Table table);

        sq::Table to_sqtable();
    };

    ColliderType type;
    AABB bounds;

    union {
        struct {
            vec2 extents;
        } aabb;
        struct {
            vec2 extents;
        } obb;
        struct {
            float radius;
        } circle;
    };

    std::vector<ContactEvent> contact_events;

    Collider() = default;
    Collider(const Options& opt);

    FUNCTION(setter)
    void set_radius(float radius) {
        assert(type == ColliderType::Circle);
        circle.radius = radius;
    }
};
