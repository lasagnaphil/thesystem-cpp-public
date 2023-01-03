//
// Created by lasagnaphil on 8/7/2021.
//

#ifndef THESYSTEM_SPRITE_RENDERER_H
#define THESYSTEM_SPRITE_RENDERER_H

#include <vector>

#include "sprite.h"
#include "camera.h"
#include "core/color.h"

#include <sokol_gfx.h>

#include "shaders/sprite.glsl.h"

class Resources;

struct SpriteVertex {
    vec2 pos;
    vec2 uv;
    rgba color;
};

class SpriteRenderer {
public:
    static constexpr int MAX_SPRITES = 65536;

    void init(Engine* engine);

    void draw(Engine* engine);

private:
    sg_shader shader;
    sg_pipeline pipeline;
    sg_pass_action pass_action;
    vs_params_t vs_params;
    sg_bindings bindings;

    std::vector<SpriteVertex> vertices;
};

#endif //THESYSTEM_SPRITE_RENDERER_H
