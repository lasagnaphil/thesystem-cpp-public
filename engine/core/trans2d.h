#pragma once

#include "core/types.h"

// Transform 2D type (shamelessly ripped off from Godot!)

struct trans2d {
    trans2d(float xx, float xy, float yx, float yy, float ox, float oy) {
        elems = {{xx, xy}, {yx, yy}, {ox, oy}};
    }
    trans2d(vec2 p_x, vec2 p_y, vec2 p_origin) {
        elems = {p_x, p_y, p_origin};
    }
    trans2d(float p_rot, vec2 p_pos) {

    }
    trans2d(float p_rot, vec2 p_scale, float p_skew, vec2 p_pos) {

    }
    trans2d() {
        elems = {{1, 0}, {0, 1}, {0, 0}};
    }

    inline float tdotx(vec2 v) const { return elems[0][0] * v.x + elems[1][0] * v.y; }
    inline float tdoty(vec2 v) const { return elems[0][1] * v.x + elems[1][1] * v.y; }

    const vec2& operator[](int p_idx) const { return elems[p_idx]; }
    vec2& operator[](int p_idx) { return elems[p_idx]; }

    inline vec2 get_axis(int p_axis) const {
        if (p_axis >= 3) {
            log_error("invalid axis index in trans2d::get_axis()!");
            return vec2(0);
        }
        return elems[p_axis];
    }

    inline void set_axis(int p_axis, vec2 p_vec) {
        if (p_axis >= 3) {
            log_error("invalid axis index in trans2d::set_axis()!");
            return;
        }
        elems[p_axis] = p_vec;
    }

    void affine_invert() {
        float det = basis_determinant();
        if (det == 0) log_error("zero determinant detected in trans2d::affine_invert()!");
        float idet = 1.0f / det;
        std::swap(elems[0][0], elems[1][1]);
        elems[0] *= vec2(idet, -idet);
        elems[1] *= vec2(-idet, idet);
        elems[2] = basis_xform(-elems[2]);
    }
    trans2d affine_inverse() const {
        trans2d inv = *this;
        inv.affine_invert();
        return inv;
    }

    void rotate(float p_phi) {
        *this = trans2d(p_phi, vec2(0)) * (*this);
    }

    float get_rotation() const {
        return glm::atan(elems[0].y, elems[0].x);
    }
    void set_rotation(float p_rot) {
        vec2 scale = get_scale();
        float cr = glm::cos(p_rot);
        float sr = glm::sin(p_rot);
        elems[0][0] = cr;
        elems[0][1] = sr;
        elems[1][0] = -sr;
        elems[1][1] = cr;
        set_scale(scale);
    }

    float get_skew() const {
        float det = basis_determinant();
        auto bx = glm::normalize(elems[0]);
        auto by = glm::normalize(elems[1]);
        return glm::acos(glm::dot(bx, glm::sign(det) * by)) - glm::half_pi<float>();
    }
    void set_skew(float p_angle) {
        float det = basis_determinant();
        float rotate_angle = glm::half_pi<float>() + p_angle;
        float cr = glm::cos(rotate_angle);
        float sr = glm::sin(rotate_angle);
        auto bx_rotated = vec2(elems[0][0] * cr - elems[0][1] * sr, elems[0][0] * sr + elems[0][1] * cr);
        elems[1] = glm::sign(det) * glm::normalize(bx_rotated) * glm::length(elems[1]);
    }

    void scale(vec2 p_scale) {
        scale_basis(p_scale);
        elems[2] *= p_scale;
    }
    void scale_basis(vec2 p_scale) {
        elems[0] *= p_scale;
        elems[1] *= p_scale;
    }

    void translate(float p_tx, float p_ty) {
        translate({p_tx, p_ty});
    }
    void translate(vec2 p_translation) {
        elems[2] += basis_xform(p_translation);
    }

    float basis_determinant() const {
        return elems[0].x * elems[1].y - elems[0].y * elems[1].x;
    }

    vec2 get_scale() const {
        float det_sign = glm::sign(basis_determinant());
        return {glm::length(elems[0]), det_sign * glm::length(elems[1])};
    }
    void set_scale(vec2 p_scale) {
        elems[0] = glm::normalize(elems[0]);
        elems[1] = glm::normalize(elems[1]);
        elems[0] *= p_scale.x;
        elems[1] *= p_scale.y;
    }

    inline vec2 get_origin() const { return elems[2]; }
    inline void set_origin(vec2 p_origin) { elems[2] = p_origin; }

    trans2d scaled(vec2 p_scale) const {
        trans2d copy = *this;
        copy.scale(p_scale);
        return copy;
    }
    trans2d basis_scaled(vec2 p_scale) const {
        trans2d copy = *this;
        copy.scale_basis(p_scale);
        return copy;
    }
    trans2d untranslated() const {
        trans2d copy = *this;
        copy.elems[2] = vec2(0);
        return copy;
    }
    trans2d translated(vec2 p_offset) const {
        trans2d copy = *this;
        copy.translate(p_offset);
        return copy;
    }
    trans2d rotated(float p_phi) const {
        trans2d copy = *this;
        copy.rotate(p_phi);
        return copy;
    }

    void orthonormalize() {
        elems[0] = glm::normalize(elems[0]);
        elems[1] = glm::normalize(elems[1] - elems[0] * glm::dot(elems[0], elems[1]));
    }
    trans2d orthonormalized() const {
        trans2d on = *this;
        on.orthonormalize();
        return on;
    }

    bool is_equal_approx(const trans2d& p_transform) const {
        constexpr float eps = 1e-12;
        return glm::any(glm::epsilonEqual(elems[0], p_transform.elems[0], eps)) &&
            glm::any(glm::epsilonEqual(elems[1], p_transform.elems[1], eps)) &&
            glm::any(glm::epsilonEqual(elems[2], p_transform.elems[2], eps));
    }

    /*
    trans2d looking_at(vec2 p_target) const {
        trans2d return_trans = trans2d(get_rotation(), get_origin());
    }
     */

    bool operator==(const trans2d& p_transform) const {
        return elems == p_transform.elems;
    }

    bool operator!=(const trans2d& p_transform) const {
        return elems != p_transform.elems;
    }

    void operator*=(const trans2d& p_transform) {
        elems[2] = xform(p_transform.elems[2]);
        auto rot = glm::mat2(elems) * glm::mat2(p_transform.elems);
        elems[0] = rot[0];
        elems[1] = rot[1];
    }
    trans2d operator*(const trans2d& p_transform) const {
        trans2d t = *this;
        t *= p_transform;
        return t;
    }

    void operator*=(float p_val) {
        elems *= p_val;
    }
    trans2d operator*(float p_val) const {
        trans2d t = *this;
        t *= p_val;
        return t;
    }

    inline vec2 basis_xform(vec2 p_vec) const { return {tdotx(p_vec), tdoty(p_vec)}; }
    inline vec2 basis_xform_inv(vec2 p_vec) const { return { glm::dot(elems[0], p_vec), glm::dot(elems[1], p_vec)}; }
    inline vec2 xform(vec2 p_vec) const { return basis_xform(p_vec) + elems[2]; }
    inline vec2 xform_inv(vec2 p_vec) const { return basis_xform_inv(p_vec - elems[2]); }

    void set_rotation_and_scale(float p_rot, vec2 p_scale) {
        float cr = glm::cos(p_rot);
        float sr = glm::sin(p_rot);
        elems[0][0] = cr * p_scale.x;
        elems[1][1] = cr * p_scale.y;
        elems[1][0] = -sr * p_scale.y;
        elems[0][1] = sr * p_scale.x;
    }
    void set_rotation_scale_and_skew(float p_rot, vec2 p_scale, float p_skew) {
        elems[0][0] = glm::cos(p_rot) * p_scale.x;
        elems[1][1] = glm::cos(p_rot + p_skew) * p_scale.y;
        elems[1][0] = -glm::sin(p_rot + p_skew) * p_scale.y;
        elems[0][1] = glm::sin(p_rot) * p_scale.x;
    }

private:
    glm::mat3x2 elems;
};