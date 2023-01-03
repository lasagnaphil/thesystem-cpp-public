//
// Created by lasagnaphil on 10/16/2021.
//

#include "sound.h"

#include "engine.h"
#include "resources.h"
#include "core/file.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.c"

#define CUTE_SOUND_IMPLEMENTATION
// #define CUTE_SOUND_FORCE_SDL
#include "cute/cute_sound.h"

void AudioSource::release() {
    cs_free_sound(&data);
}

Sound::Sound() {
    void* hwnd = const_cast<void*>(sapp_win32_get_hwnd());
    ctx = cs_make_context(hwnd, 44100, 4096, 0, nullptr);
    cs_spawn_mix_thread(ctx);
    cs_thread_sleep_delay(ctx, 5);
}

Sound::~Sound() {
    cs_shutdown_context(ctx);
}

Ref<AudioSource> Sound::load_wav(const std::string& filename) {
    auto source = Engine::instance().get_resources()->new_item<AudioSource>();
    auto ptr = source.get();
    auto buf = load_file_to_buffer(filename.c_str());
    cs_read_mem_wav(buf.data(), buf.size(), &ptr->data);
    ptr->loaded = true;
    return source;
}

Ref<AudioSource> Sound::load_ogg(const std::string& filename) {
    auto source = Engine::instance().get_resources()->new_item<AudioSource>();
    auto ptr = source.get();
    auto buf = load_file_to_buffer(filename.c_str());
    cs_read_mem_ogg(buf.data(), buf.size(), &ptr->data);
    ptr->loaded = true;
    return source;
}

Ref<AudioInstance> Sound::make_instance(Ref<AudioSource> source) {
    auto inst = Engine::instance().get_resources()->new_item<AudioInstance>();
    auto ptr = inst.get();
    ptr->data = cs_make_playing_sound(&source.get()->data);
    ptr->loaded = true;
    return inst;
}

void Sound::play(Ref<AudioInstance> inst) {
    auto p = inst.get();
    if (p->loaded) {
        cs_insert_sound(ctx, &p->data);
    }
}

void Sound::stop_all() {
    Engine::instance().get_resources()->foreach<AudioInstance>([](AudioInstance& inst) {
        if (inst.loaded) {
            cs_stop_sound(&inst.data);
        }
    });
}
