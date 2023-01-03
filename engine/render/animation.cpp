//
// Created by lasagnaphil on 10/24/2021.
//

#include "animation.h"

// #define CUTE_ASEPRITE_IMPLEMENTATION
// #include "cute/cute_aseprite.h"

#include "engine.h"
#include "core/file.h"
#include "squirrel/vm.h"

#include <rapidjson/document.h>

Animation::Options::Options(sq::Table args) {
    auto& vm = *Engine::instance().get_vm();
    json = vm.get<std::string>(args, "json");
}

Animation::Animation(const Options &opt) {
    if (!opt.json.empty()) {
        load_from_json(opt.json);
    }
    else {
        log_error("Not enough arguments for Animation!");
    }
}

void Animation::load_from_json(const std::string &filename) {
    auto buf = load_file_to_buffer(filename.c_str());
    if (buf.empty()) {
        log_error("Failed to load animation {}!", filename.c_str());
    }

    rapidjson::Document doc;
    doc.Parse(buf.data(), buf.size());

    auto& el_meta = doc["meta"];
    auto image_path = get_parent_dir(filename) + "/" + el_meta["image"].GetString();
    tex_ref = Texture::from_image_file(image_path);

    ivec2 size = {el_meta["size"]["w"].GetInt(), el_meta["size"]["h"].GetInt()};

    auto& el_frame_tags = el_meta["frameTags"];
    for (auto& el_tag : el_frame_tags.GetArray()) {
        Tag tag;
        tag.name = el_tag["name"].GetString();
        tag.from = el_tag["from"].GetInt();
        tag.to = el_tag["to"].GetInt();
        tags.push_back(tag);
    }

    auto& el_frames = doc["frames"];
    for (auto& el_frame_entry : el_frames.GetObject()) {
        auto& el_frame = el_frame_entry.value["frame"];
        Frame frame;
        frame.srcrect = {
            el_frame["x"].GetInt(),
            el_frame["y"].GetInt(),
            el_frame["w"].GetInt(),
            el_frame["h"].GetInt(),
        };
        frame.duration = el_frame_entry.value["duration"].GetInt();
        frames.push_back(frame);
    }

    srcrect = irect(0, 0, size.x, size.y);
}

void Animation::update(float dt) {
    auto& state = tags[cur_state];
    if (is_playing) {
        time_elapsed_since_tick += dt;
        while (time_elapsed_since_tick >= 0.001f * frames[cur_frame].duration) {
            time_elapsed_since_tick -= 0.001f * frames[cur_frame].duration;
            cur_frame++;
            if (cur_frame >= state.frame_len()) {
                cur_frame = 0;
            }
        }
    }

    int cur_global_frame = state.from + cur_frame;
    srcrect = frames[cur_global_frame].srcrect;
}

void Animation::set_state(const std::string& state_name) {
    cur_state = tag_map[state_name.c_str()];
    cur_frame = 0;
}

void Animation::set_frame(int frame_idx) {
    assert(frame_idx >= 0 && frame_idx < tags[cur_state].frame_len());
    cur_frame = frame_idx;
}
