//
// Created by lasagnaphil on 8/14/2021.
//

#ifndef THESYSTEM_RECT_H
#define THESYSTEM_RECT_H

#include <core/types.h>

class Engine;

template <class T>
struct trect {
    glm::tvec2<T> pos;
    glm::tvec2<T> size;

    trect() = default;

    trect(glm::tvec2<T> pos, glm::tvec2<T> size) : pos(pos), size(size) {};
    trect(T pos_x, T pos_y, T size_x, T size_y) : pos(pos_x, pos_y), size(size_x, size_y) {};
    template <class U>
    explicit trect(const trect<U>& rect) : pos(rect.pos), size(rect.size) {}

    glm::tvec2<T> get_center() {
        return pos + size/T(2);
    }

    void set_center(glm::tvec2<T> center) {
        pos = center - size/T(2);
    }

    glm::tvec2<T> v0() {
        return pos;
    };

    glm::tvec2<T> v1() {
        return {pos.x + size.x, pos.y};
    }

    glm::tvec2<T> v2() {
        return {pos.x, pos.y + size.y};
    }

    glm::tvec2<T> v3() {
        return pos + size;
    }
};

template <class T>
inline bool intersects(const trect<T>& aabb1, const trect<T>& aabb2) {
    aabb1.pos.x < aabb2.pos.
}

using rect = trect<float>;
using irect = trect<int>;

#endif //THESYSTEM_RECT_H
