#pragma once

#include "core/types.h"
#include "core/reflect.h"
#include "render/node.h"

CLASS(Resource, Node) KinematicBody : public Node {
private:
    float margin = 0.1;
    std::vector<Ref<Collider>> colliders;

public:
    struct Options {
        float margin = 0.1;
        Options() = default;
        Options(sq::Table table);
    };

    KinematicBody() = default;
    KinematicBody(const Options& opt) : margin(opt.margin) {}

    void move_and_collide(vec2 dx);
    void move_and_slide(vec2 dx);

    void add_collider(Ref<Collider> col_ref);

    void add_child_colliders();
    void add_child_colliders_recursive();
};