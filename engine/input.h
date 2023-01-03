//
// Created by lasagnaphil on 10/16/2021.
//

#ifndef THESYSTEM_INPUT_H
#define THESYSTEM_INPUT_H

#include <glm/ext.hpp>
#include <sokol_app.h>

class Engine;

struct Input {
    uint8_t curr_keys[SAPP_MAX_KEYCODES] = {};
    uint8_t prev_keys[SAPP_MAX_KEYCODES] = {};

    uint32_t prev_mouse_button;
    uint32_t curr_mouse_button;
    glm::ivec2 mouse_pos;
    glm::ivec2 mouse_movepos;

    void key_event(const sapp_event* ev);

    void after_update();

    bool is_key_pressed(uint32_t key);
    bool is_key_entered(uint32_t key);
    bool is_key_exited(uint32_t key);

    bool is_mouse_pressed(uint8_t button);
    bool is_mouse_entered(uint8_t button);
    bool is_mouse_exited(uint8_t button);

    glm::ivec2 get_mouse_pos();
    glm::ivec2 get_rel_mouse_pos();
};

#endif //THESYSTEM_INPUT_H
