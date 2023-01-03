#pragma once

#include "engine.h"
#include "core/trans2d.h"
#include "core/reflect.h"

// Node type (shamelessly ripped off from Godot!)

namespace Layers {
enum Layers : uint16_t {
    Background = 0,
    Sprite = 1,
    Foreground = 2,
    UI = 3,
};
}

CLASS(Resource, Root) Node {
public:
    CLASS(OptionFor=Node) Options {
    public:
        vec2 pos = {0, 0};
        vec2 scale = {1, 1};
        float rot = 0;
        uint16_t layer = 0;
        uint16_t z_index = 0;
        bool update_enabled = true;
        bool render_enabled = true;

        Options() = default;
        Options(sq::Table table);

        sq::Table to_sqtable();
    };
protected:
    Ref<Node> parent = {};
    std::vector<Ref<Node>> children;

    vec2 pos = {0, 0};
    float rot = 0;
    vec2 scale = {1, 1};
    float skew = 0;
    uint16_t layer = 0;
    uint16_t z_index = 0;
    bool z_relative = true;

    bool update_enabled = true;
    bool render_enabled = true;

    trans2d transform; // local
    trans2d global_transform; // global

    mutable bool _xform_dirty = true;
    mutable bool _global_xform_dirty = true;

public:
    friend class Collider;

    Node() = default;
    Node(const Options& opt);

    Ref<Node> get_self() const;

    FUNCTION(getter)
    Ref<Node> get_parent() const { return parent; }
    FUNCTION(getter)
    std::vector<Ref<Node>> get_children() const { return children; }

    FUNCTION(setter)
    void set_parent(Ref<Node> parent_ref);
    FUNCTION()
    void add_child(Ref<Node> child_ref);
    FUNCTION()
    void remove_child(Ref<Node> child_ref);

    FUNCTION()
    bool is_update_enabled() const { return update_enabled; }
    FUNCTION()
    bool is_render_enabled() const { return render_enabled; }
    FUNCTION()
    void set_update_enable(bool p_enable);
    FUNCTION()
    void set_render_enable(bool p_enable);
    FUNCTION()
    void set_enable(bool p_enable) {
        set_update_enable(p_enable);
        set_render_enable(p_enable);
    }

    FUNCTION(getter)
    vec2 get_pos() const {
        const_cast<Node *>(this)->_update_xform_values();
        return pos;
    }
    FUNCTION(getter)
    float get_pos_x() const {
        const_cast<Node *>(this)->_update_xform_values();
        return pos.x;
    }
    FUNCTION(getter)
    float get_pos_y() const {
        const_cast<Node *>(this)->_update_xform_values();
        return pos.y;
    }
    FUNCTION(getter)
    float get_rot() const {
        const_cast<Node *>(this)->_update_xform_values();
        return rot;
    }
    FUNCTION(getter)
    float get_skew() const {
        const_cast<Node *>(this)->_update_xform_values();
        return skew;
    }
    FUNCTION(getter)
    vec2 get_scale() const {
        const_cast<Node *>(this)->_update_xform_values();
        return scale;
    }
    FUNCTION(getter)
    trans2d get_global_trans() const {
        const_cast<Node *>(this)->_update_global_xform();
        return global_transform;
    }
    FUNCTION(getter)
    vec2 get_global_pos() const {
        const_cast<Node *>(this)->_update_global_xform();
        return global_transform.get_origin();
    }
    FUNCTION(getter)
    float get_global_rot() const {
        const_cast<Node *>(this)->_update_global_xform();
        return global_transform.get_rotation();
    }
    FUNCTION(getter)
    vec2 get_global_scale() const {
        const_cast<Node *>(this)->_update_global_xform();
        return global_transform.get_scale();
    }
    FUNCTION()
    bool is_z_relative() const {
        return z_relative;
    }
    FUNCTION(getter)
    uint16_t get_layer() const {
        return layer;
    }
    FUNCTION(getter)
    uint16_t get_z_index() const {
        return z_index;
    }

    FUNCTION(setter)
    void set_trans(const trans2d& p_transform) {
        transform = p_transform;
        _xform_dirty = true;
        _update_xform();
    }
    FUNCTION(setter)
    void set_pos(vec2 p_pos) {
        const_cast<Node *>(this)->_update_xform_values();
        pos = p_pos;
        _update_xform();
    }
    // void set_pos(float p_x, float p_y) { set_pos({p_x, p_y}); }
    FUNCTION(setter)
    void set_pos_x(float p_x) {
        const_cast<Node *>(this)->_update_xform_values();
        pos.x = p_x;
        _update_xform();
    }
    FUNCTION(setter)
    void set_pos_y(float p_y) {
        const_cast<Node *>(this)->_update_xform_values();
        pos.y = p_y;
        _update_xform();
    }
    FUNCTION(setter)
    void set_rot(float p_radians) {
        const_cast<Node *>(this)->_update_xform_values();
        rot = p_radians;
        _update_xform();
    }
    FUNCTION(setter)
    void set_skew(float p_radians) {
        const_cast<Node *>(this)->_update_xform_values();
        skew = p_radians;
        _update_xform();
    }
    FUNCTION(setter)
    void set_scale(vec2 p_scale) {
        const_cast<Node *>(this)->_update_xform_values();
        scale = p_scale;
        _update_xform();
    }
    FUNCTION(setter)
    void set_global_trans(const trans2d& p_transform) {
        if (parent) {
            set_trans(parent.get()->get_global_trans().affine_inverse() * p_transform);
        }
        else {
            set_trans(p_transform);
        }
    }
    FUNCTION(setter)
    void set_global_pos(vec2 p_pos) {
        if (parent) {
            set_pos(parent.get()->get_global_trans().affine_inverse().xform(p_pos));
        }
        else {
            set_pos(p_pos);
        }
    }
    FUNCTION(setter)
    void set_global_rot(float p_radians) {
        if (parent) {
            float parent_global_rot = parent.get()->get_global_trans().get_rotation();
            set_rot(p_radians - parent_global_rot);
        }
        else {
            set_rot(p_radians);
        }
    }
    FUNCTION(setter)
    void set_global_scale(vec2 p_scale) {
        if (parent) {
            vec2 parent_global_scale = parent.get()->get_global_trans().get_scale();
            set_scale(p_scale / parent_global_scale);
        }
        else {
            set_scale(p_scale);
        }
    }
    FUNCTION(setter)
    void set_layer(uint16_t p_layer) {
        layer = p_layer;
    }
    FUNCTION(setter)
    void set_z_index(uint16_t p_z) {
        z_index = p_z;
    }
    FUNCTION(setter)
    void set_z_as_relative(bool p_enabled) {
        z_relative = p_enabled;
    }

    FUNCTION()
    void rotate(float p_radians) {
        set_rot(get_rot() + p_radians);
    }
    FUNCTION()
    void move_x(float p_delta, bool p_scaled = false) {
        trans2d t = get_global_trans();
        vec2 m = t[0];
        if (!p_scaled) m = glm::normalize(m);
        set_pos(t[2] + m * p_delta);
    }
    FUNCTION()
    void move_y(float p_delta, bool p_scaled = false) {
        trans2d t = get_global_trans();
        vec2 m = t[1];
        if (!p_scaled) m = glm::normalize(m);
        set_pos(t[2] + m * p_delta);
    }
    FUNCTION()
    void translate(vec2 p_amount) {
        set_pos(get_pos() + p_amount);
    }
    FUNCTION()
    void global_translate(vec2 p_amount) {
        set_global_pos(get_global_pos() + p_amount);
    }
    FUNCTION()
    void apply_scale(vec2 p_amount) {
        set_scale(get_scale() * p_amount);
    }

    void _notify_transform();

    void _update_xform_values();

    void _update_xform();

    void _update_global_xform();

    trans2d& _get_global_trans_raw() { return global_transform; }
    const trans2d& _get_global_trans_raw() const { return global_transform; }
};
