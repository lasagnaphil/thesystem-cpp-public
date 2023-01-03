//
// Created by lasagnaphil on 10/16/2021.
//

#include "input.h"
#include "engine.h"

void Input::key_event(const sapp_event *ev) {
    switch (ev->type) {
        case SAPP_EVENTTYPE_KEY_DOWN: {
            curr_keys[ev->key_code] = 1;
        } break;
        case SAPP_EVENTTYPE_KEY_UP: {
            curr_keys[ev->key_code] = 0;
        } break;
        case SAPP_EVENTTYPE_MOUSE_MOVE: {
            mouse_pos = {ev->mouse_x, ev->mouse_y};
            mouse_movepos = {ev->mouse_dx, ev->mouse_dy};
        } break;
        case SAPP_EVENTTYPE_MOUSE_UP: {
            curr_mouse_button |= ev->mouse_button;
        } break;
        case SAPP_EVENTTYPE_MOUSE_DOWN: {
            curr_mouse_button &= (~ev->mouse_button);
        } break;
    }
}

void Input::after_update() {
    prev_mouse_button = curr_mouse_button;
    memcpy(prev_keys, curr_keys, SAPP_MAX_KEYCODES);
}

bool Input::is_key_pressed(uint32_t key){
    return curr_keys[key];
}

bool Input::is_key_entered(uint32_t key) {
    return curr_keys[key] && !prev_keys[key];
}

bool Input::is_key_exited(uint32_t key) {
    return !curr_keys[key] && prev_keys[key];
}

bool Input::is_mouse_pressed(uint8_t button) {
    return (curr_mouse_button & button) != 0;
}

bool Input::is_mouse_entered(uint8_t button) {
    return (curr_mouse_button & button) != 0 &&
           (curr_mouse_button & button) == 0;
}

bool Input::is_mouse_exited(uint8_t button) {
    return (curr_mouse_button & button) == 0 &&
           (prev_mouse_button & button) != 0;
}

glm::ivec2 Input::get_mouse_pos() {
    return mouse_pos;
}

glm::ivec2 Input::get_rel_mouse_pos() {
    return mouse_movepos;
}
