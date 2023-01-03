//
// Created by lasagnaphil on 10/16/2021.
//

#ifndef THESYSTEM_SOUND_H
#define THESYSTEM_SOUND_H

#include "core/reflect.h"
#include "resource_pool.h"

#include "cute/cute_sound.h"

#include <string>

class Engine;

CLASS(Resource) AudioInstance {
public:
    friend class AudioSource;
    friend class Sound;
private:
    cs_playing_sound_t data;
    bool loaded = false;
};

CLASS(Resource) AudioSource {
public:
    friend class Sound;
    void release();
private:
    cs_loaded_sound_t data;
    bool loaded = false;
};

class Sound {
public:
    friend class AudioInstance;
    friend class AudioSource;

    Sound();
    ~Sound();

    Ref<AudioSource> load_wav(const std::string& filename);
    Ref<AudioSource> load_ogg(const std::string& filename);

    void play(Ref<AudioInstance> inst);

    void stop_all();

    Ref<AudioInstance> make_instance(Ref<AudioSource> source);

private:
    cs_context_t* ctx;
};

#endif //THESYSTEM_SOUND_H
