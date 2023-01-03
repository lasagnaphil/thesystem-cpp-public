
#include "api.h"

#include "engine.h"
#include "input.h"
#include "sound.h"
#include "reflection.h"
#include "render/node.h"
#include "render/texture.h"
#include "render/sprite.h"
#include "render/font.h"
#include "render/text.h"
#include "render/animation.h"
#include "squirrel/scriptable.h"
#include "squirrel/scriptable_impl.h"
#include "squirrel/vm.h"
#include "collision/kinematic_body.h"

#include "imgui.h"
#include <sokol_gp.h>

template <class T>
Ref<T> constructor(sq::Table table) {
    return Engine::instance().get_resources()->new_item<T>(typename T::Options(table));
}

void register_api() {
    auto& engine = Engine::instance();
    auto& vm = *engine.get_vm();
    auto& res = *engine.get_resources();

    // App
    {
        vm.add_func("app_frame_duration", +[]() { return (float)sapp_frame_duration(); });
        vm.add_func("app_frame_count", sapp_frame_count);
        vm.add_func("app_width", +[]() { return (int)(sapp_widthf() / sapp_dpi_scale()); });
        vm.add_func("app_height", +[]() { return (int)(sapp_heightf() / sapp_dpi_scale()); });
        vm.add_func("app_fps", +[]() {
            return Engine::instance().get_fps();
        });
    }

    // Debug
    {
        std::function<void()> sq_print_stack_fn = [&]() { sq_print_stack(vm.handle()); };
        vm.add_func("sq_print_stack", sq_print_stack_fn);
    }

    // Color
    {
        vm.add_func("color_from_hex", &Colors::from_hex_string);
    }

    // sokol-gp
    {

#define ADD_GP_FUNC(name) vm.add_func("gp_"#name, sgp_##name);
        ADD_GP_FUNC(begin)
        ADD_GP_FUNC(flush)
        ADD_GP_FUNC(end)
        ADD_GP_FUNC(project)
        ADD_GP_FUNC(reset_project)
        ADD_GP_FUNC(push_transform)
        ADD_GP_FUNC(pop_transform)
        ADD_GP_FUNC(reset_transform)
        ADD_GP_FUNC(translate)
        ADD_GP_FUNC(rotate)
        ADD_GP_FUNC(rotate_at)
        ADD_GP_FUNC(scale)
        ADD_GP_FUNC(scale_at)

        ADD_GP_FUNC(viewport)
        ADD_GP_FUNC(reset_viewport)
        ADD_GP_FUNC(scissor)
        ADD_GP_FUNC(reset_scissor)
        ADD_GP_FUNC(reset_state)

        ADD_GP_FUNC(clear)
        ADD_GP_FUNC(draw_point)
        ADD_GP_FUNC(draw_line)
        ADD_GP_FUNC(draw_filled_triangle)
        ADD_GP_FUNC(draw_filled_rect)
        ADD_GP_FUNC(draw_textured_rect)

#undef ADD_GP_FUNC

    }

    // Input
    {
        auto cls = vm.add_class<Input>("_Input");
        vm.add_method(cls, "is_key_pressed", &Input::is_key_pressed);
        vm.add_method(cls, "is_key_entered", &Input::is_key_entered);
        vm.add_method(cls, "is_key_exited", &Input::is_key_exited);
        vm.add_method(cls, "is_mouse_pressed", &Input::is_mouse_pressed);
        vm.add_method(cls, "is_mouse_entered", &Input::is_mouse_entered);
        vm.add_method(cls, "is_mouse_exited", &Input::is_mouse_exited);
        vm.add_method(cls, "get_mouse_pos", &Input::get_mouse_pos);
        vm.add_method(cls, "get_rel_mouse_pos", &Input::get_rel_mouse_pos);

        auto obj = vm.new_instance_from_ptr(engine.get_input());
        vm.add_global("Input", obj.cast<sq::Object>());

        vm.add_enum("Scancode", {
            {"Left", SAPP_KEYCODE_LEFT},
            {"Right", SAPP_KEYCODE_RIGHT},
            {"Up", SAPP_KEYCODE_UP},
            {"Down", SAPP_KEYCODE_DOWN},
            {"Z", SAPP_KEYCODE_Z},
            {"X", SAPP_KEYCODE_X},
            {"C", SAPP_KEYCODE_C},
            {"Enter", SAPP_KEYCODE_ENTER},
            {"Space", SAPP_KEYCODE_SPACE},
        });
    }

    // Sound
    {
        auto sound = vm.add_class<Sound>("_Sound");
        auto obj = vm.new_instance_from_ptr(engine.get_sound());
        vm.add_global("Sound", obj.cast<sq::Object>());

        auto audio_source = vm.add_class<AudioSource>("AudioSource");
        auto audio_inst = vm.add_class<AudioInstance>("AudioInstance");

        vm.add_method(sound, "load_wav", &Sound::load_wav);
        vm.add_method(sound, "load_ogg", &Sound::load_ogg);
        vm.add_method(sound, "make_instance", &Sound::make_instance);
        vm.add_method(sound, "play", &Sound::play);
        vm.add_method(audio_source, "release", &AudioSource::release);
    }

    // Texture
    {
        auto cls = vm.add_class<Texture>("Texture", &Texture::from_image);
    }

    // Font
    {
        auto font = vm.add_class<Font>("Font", &Font::load_xml);
    }

    // Node
    {
        auto cls = vm.add_class<Node>("Node", constructor<Node>);
        vm.add_method(cls, "set_parent", &Node::set_parent);
        vm.add_method(cls, "add_child", &Node::add_child);
        vm.add_method(cls, "remove_child", &Node::remove_child);
        vm.add_method(cls, "is_update_enabled", &Node::is_update_enabled);
        vm.add_method(cls, "is_render_enabled", &Node::is_render_enabled);
        vm.add_method(cls, "set_update_enable", &Node::set_update_enable);
        vm.add_method(cls, "set_render_enable", &Node::set_render_enable);
        vm.add_method(cls, "set_enable", &Node::set_enable);
        vm.add_method(cls, "get_pos", &Node::get_pos);
        vm.add_method(cls, "get_pos_x", &Node::get_pos_x);
        vm.add_method(cls, "get_pos_y", &Node::get_pos_y);
        vm.add_method(cls, "get_rot", &Node::get_rot);
        vm.add_method(cls, "get_skew", &Node::get_skew);
        vm.add_method(cls, "get_scale", &Node::get_scale);
        vm.add_method(cls, "set_pos", &Node::set_pos);
        vm.add_method(cls, "set_pos_x", &Node::set_pos_x);
        vm.add_method(cls, "set_pos_y", &Node::set_pos_y);
        vm.add_method(cls, "set_rot", &Node::set_rot);
        vm.add_method(cls, "set_skew", &Node::set_skew);
        vm.add_method(cls, "set_scale", &Node::set_scale);
        vm.add_method(cls, "rotate", &Node::rotate);
    }


    // Sprite
    {
        vm.add_enum("Layers", {
                {"Background", Layers::Background},
                {"Sprite", Layers::Sprite},
                {"Foreground", Layers::Foreground},
                {"UI", Layers::UI}
        });

        auto cls = vm.add_class<Sprite, Node>("Sprite", constructor<Sprite>);
        vm.add_method(cls, "get_width", &Sprite::get_width);
        vm.add_method(cls, "get_height", &Sprite::get_height);
    }
    {
        using SSprite = Scriptable<Sprite>;
        auto cls = vm.add_class<SSprite, Sprite>("ScriptableSprite", constructor<SSprite>);
    }

    // Text
    {
        auto cls = vm.add_class<Text, Node>("Text", constructor<Text>);
        vm.add_method(cls, "get_contents", &Text::get_contents);
        vm.add_method(cls, "set_contents", &Text::set_contents);
        vm.add_method(cls, "get_font", &Text::get_font);
        vm.add_method(cls, "set_font", &Text::set_font);
        vm.add_method(cls, "get_layer", &Text::get_layer);
        vm.add_method(cls, "set_layer", &Text::set_layer);
    }
    {
        using SText = Scriptable<Text>;
        auto cls = vm.add_class<SText, Text>("ScriptableText", constructor<SText>);
    }

    // Animation
    {
        auto cls = vm.add_class<Animation, Sprite>("Animation", constructor<Animation>);
        vm.add_method(cls, "set_state", &Animation::set_state);
        vm.add_method(cls, "set_frame", &Animation::set_frame);
    }
    {
        using SAnimation = Scriptable<Animation>;
        auto cls = vm.add_class<SAnimation, Animation>("ScriptableAnimation", constructor<SAnimation>);
    }

    // KinematicBody
    {
        auto cls = vm.add_class<KinematicBody, Node>("KinematicBody", constructor<KinematicBody>);
        vm.add_method(cls, "move_and_collide", &KinematicBody::move_and_collide);
        vm.add_method(cls, "move_and_slide", &KinematicBody::move_and_slide);
        vm.add_method(cls, "add_collider", &KinematicBody::add_collider);
        vm.add_method(cls, "add_child_colliders", &KinematicBody::add_child_colliders);
        vm.add_method(cls, "add_child_colliders_recursive", &KinematicBody::add_child_colliders_recursive);
    }
    {
        using SKinematicBody = Scriptable<KinematicBody>;
        auto cls = vm.add_class<SKinematicBody, KinematicBody>("ScriptableKinematicBody", constructor<SKinematicBody>);
    }
}
