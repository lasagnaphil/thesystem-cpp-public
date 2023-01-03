//
// Created by lasagnaphil on 8/7/2021.
//

#include "sprite_renderer.h"

#include <sokol_gfx.h>

#include "resources.h"
#include "camera.h"
#include "engine.h"
#include "core/timer.h"
#include "render/animation.h"

#include <algorithm>

#include <Tracy.hpp>

inline uint64_t get_sprite_order_id(uint32_t texture_id, uint16_t layer, uint16_t order) {
    return ((uint64_t)layer << 48) | ((uint64_t)order << 32) | texture_id;
}

void SpriteRenderer::init(Engine* engine) {
    ZoneScoped
    shader = sg_make_shader(sprite_shader_desc(sg_query_backend()));

    pipeline = sg_make_pipeline(sg_pipeline_desc {
        .shader = shader,
        .layout = {
            .attrs = {
                {.format = SG_VERTEXFORMAT_FLOAT2},
                {.format = SG_VERTEXFORMAT_FLOAT2},
                {.format = SG_VERTEXFORMAT_UBYTE4N},
            }
        },
        .colors = {
            {
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .op_rgb = SG_BLENDOP_ADD,
                    .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .op_alpha = SG_BLENDOP_ADD,
                }
            }
        },
        .label = "SpriteRenderer",
    });

    bindings = {};
    bindings.vertex_buffers[0] = sg_make_buffer(sg_buffer_desc {
        .size = 6*sizeof(SpriteVertex)*MAX_SPRITES,
        .usage = SG_USAGE_STREAM,
    });
}

void SpriteRenderer::draw(Engine* engine) {
    ZoneScoped
    auto res = engine->get_resources();
    auto& sprites = res->get_pool<Sprite>();
    auto& textures = res->get_pool<Texture>();
    if (sprites.size() == 0) return;

    auto camera = engine->get_camera();
    struct SpriteEntry {
        Ref<Sprite> sprite_ref;
        uint64_t sprite_order_id;
    };
    std::vector<SpriteEntry> sorted_sprites;
    sorted_sprites.reserve(sprites.size());

    res->foreach_ref<Sprite>([&](Ref<Sprite> sprite_ref, Sprite& sprite) {
        if (!sprite.is_render_enabled()) return;
        SpriteEntry entry;
        entry.sprite_ref = sprite_ref;
        uint32_t texture_index = textures.get_index(sprite.tex_ref);
        entry.sprite_order_id = get_sprite_order_id(texture_index, sprite.layer, sprite.z_index);
        sorted_sprites.push_back(entry);
    });

    {
        ZoneScopedN("Sprite Sorting")

        std::sort(sorted_sprites.begin(), sorted_sprites.end(), [](const SpriteEntry& e1, const SpriteEntry& e2) {
            return e1.sprite_order_id < e2.sprite_order_id;
        });
    }

    mat4 model_mat = camera->get_model_mat();
    mat4 proj_mat = camera->get_proj_mat();
    mat4 trans_mat = model_mat * proj_mat;

    std::vector<int> texture_change_indices;
    std::vector<Ref<Texture>> render_textures;

    {
        ZoneScopedN("Generate VBO Mesh")

        vertices.reserve(sorted_sprites.size() * 6);
        for (auto& entry : sorted_sprites) {
            auto& sprite = *entry.sprite_ref.get();
            auto& tex = *sprite.tex_ref.get();
            auto tex_info = sg_query_image_info(tex.img);
            vec2 tex_size = {tex_info.width, tex_info.height};
            trans2d trans = sprite.get_global_trans();
            rect local_rect = {-sprite.origin, sprite.srcrect.size};
            vec2 v0 = trans.xform(local_rect.v0());
            vec2 v1 = trans.xform(local_rect.v1());
            vec2 v2 = trans.xform(local_rect.v2());
            vec2 v3 = trans.xform(local_rect.v3());
            {
                SpriteVertex vertex;
                vertex.pos = v0;
                vertex.uv = vec2(sprite.srcrect.v0()) / tex_size;
                vertex.color = sprite.color;
                vertices.push_back(vertex);
            }
            {
                SpriteVertex vertex;
                vertex.pos = v1;
                vertex.uv = vec2(sprite.srcrect.v1()) / tex_size;
                vertex.color = sprite.color;
                vertices.push_back(vertex);
            }
            {
                SpriteVertex vertex;
                vertex.pos = v2;
                vertex.uv = vec2(sprite.srcrect.v2()) / tex_size;
                vertex.color = sprite.color;
                vertices.push_back(vertex);
            }
            {
                SpriteVertex vertex;
                vertex.pos = v1;
                vertex.uv = vec2(sprite.srcrect.v1()) / tex_size;
                vertex.color = sprite.color;
                vertices.push_back(vertex);
            }
            {
                SpriteVertex vertex;
                vertex.pos = v3;
                vertex.uv = vec2(sprite.srcrect.v3()) / tex_size;
                vertex.color = sprite.color;
                vertices.push_back(vertex);
            }
            {
                SpriteVertex vertex;
                vertex.pos = v2;
                vertex.uv = vec2(sprite.srcrect.v2()) / tex_size;
                vertex.color = sprite.color;
                vertices.push_back(vertex);
            }
        }

        uint64_t cur_sprite_order_id = -1;
        for (int i = 0; i < sorted_sprites.size(); i++) {
            auto& entry = sorted_sprites[i];
            if (cur_sprite_order_id != entry.sprite_order_id) {
                cur_sprite_order_id = entry.sprite_order_id;
                auto& sprite = *entry.sprite_ref.get();
                texture_change_indices.push_back(i);
                render_textures.push_back(sprite.tex_ref);
            }
        }
        texture_change_indices.push_back(sorted_sprites.size());

    }

    {
        ZoneScopedN("GPU Render")
        sg_apply_pipeline(pipeline);

        if (!vertices.empty()) {
            sg_update_buffer(bindings.vertex_buffers[0], {vertices.data(), sizeof(SpriteVertex)*vertices.size()});
        }

        vs_params.u_trans = trans_mat;
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, {&vs_params, sizeof(vs_params)});

        for (int i = 0; i < texture_change_indices.size() - 1; i++) {
            int start_idx = texture_change_indices[i];
            int end_idx = texture_change_indices[i+1];
            auto tex = render_textures[i].get();
            bindings.fs_images[SLOT_u_tex] = tex->img;
            sg_apply_bindings(bindings);
            sg_draw(6*start_idx, 6*(end_idx - start_idx), 1);
        }
    }

    vertices.clear();
}

