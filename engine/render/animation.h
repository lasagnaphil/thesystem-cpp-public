//
// Created by lasagnaphil on 10/24/2021.
//

#ifndef THESYSTEM_ANIMATION_H
#define THESYSTEM_ANIMATION_H

#include <vector>

#include "sprite.h"
#include "core/reflect.h"
#include "core/string_map.h"
#include "cute/cute_aseprite.h"

class Engine;

CLASS(Resource) Animation : public Sprite {
    struct Tag {
        enum class PlayDir : uint8_t {
            Forward, Reverse, PingPong
        };

        std::string name;
        int from;
        int to;
        PlayDir direction;

        int frame_len() const {
            return to - from;
        }
    };

    struct Frame {
        irect srcrect;
        int duration;
    };

    std::vector<Tag> tags;
    string_map<int> tag_map;

    int width, height;
    int cur_state;
    int cur_frame;
    std::vector<Frame> frames;
    float time_elapsed_since_tick;

    bool is_playing = false;

public:
    CLASS(OptionFor=Animation) Options {
    public:
        std::string json;

        Options() = default;
        Options(sq::Table args);
    };

    Animation() = default;
    Animation(const Options& opt);

    void load_from_json(const std::string& filename);

    void update(float dt);

    void set_state(const std::string& state_name);
    void set_frame(int frame_idx);

    void play() { is_playing = true; }
    void stop() { is_playing = false; }
};

#endif //THESYSTEM_ANIMATION_H
