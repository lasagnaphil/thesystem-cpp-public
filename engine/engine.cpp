//
// Created by lasagnaphil on 2021-07-25.
//

#include "engine.h"

#include "api.h"
#include "script.h"
#include "input.h"
#include "sound.h"
#include "core/log.h"
#include "core/color.h"
#include "render/tilemap.h"
#include "render/camera.h"
#include "render/font.h"
#include "render/text.h"
#include "render/animation.h"
#include "render/sprite_renderer.h"
#include "render/sprite.h"
#include "squirrel/vm.h"
#include "collision/collision_manager.h"

#include "physfs.h"
#include "pugixml.hpp"

#include "imgui.h"

#include "sokol/sokol_impl.h"

#include <Tracy.hpp>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include "core/windows_utils.h"
#endif

Engine* Engine::inst = nullptr;

Engine::Engine(int argc, char** argv) : argc(argc), argv(argv) {
}

Engine::~Engine() {
}

void Engine::push_scene(const char* scene_script_path) {
    Scene scene(this, scene_script_path);
    scene.load();
    scene_stack.push_back(scene);
}

void sapp_init_cb() {
    Engine::instance().base_init();
}

void sapp_update_cb() {
    auto& engine = Engine::instance();
    double dt = sapp_frame_duration();
    engine.base_update(dt);
    engine.base_render();
}

void sapp_event_cb(const sapp_event* ev) {
    Engine::instance().base_event(ev);
}

void sapp_cleanup_cb() {
    Engine::instance().base_release();
}

sapp_desc Engine::get_app_desc() {
    sapp_desc desc = {
            .init_cb = sapp_init_cb,
            .frame_cb = sapp_update_cb,
            .cleanup_cb = sapp_cleanup_cb,
            .event_cb = sapp_event_cb,
            .width = game_width,
            .height = game_height,
            .high_dpi = true,
    };
    return desc;
}

void Engine::base_pre_init() {
    ZoneScoped

    // Initialize logger
    log_init("log.txt");

    // Initialize PhysFS
    PHYSFS_init(argv[0]);

    // Check if assets are embedded in executable

#ifdef EXE_EMBEDDED_ASSETS
#ifdef _WIN32
    auto exe_path = windows_get_current_exe_path_utf16();
    HANDLE exe_file = CreateFileW(exe_path.data(),
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  NULL,
                                  NULL);
    if (exe_file == INVALID_HANDLE_VALUE) {
        log_error("Windows error: {}", windows_get_last_error_utf8());
        std::abort();
    }
    DWORD exe_size = GetFileSize(exe_file, NULL);

    DWORD exe_data_segment_length;
    SetFilePointer(exe_file, -4, NULL, FILE_END);
    ReadFile(exe_file, (void*)&exe_data_segment_length, 4, NULL, NULL);
    DWORD exe_data_segment_start = exe_size - 4 - exe_data_segment_length;
    SetFilePointer(exe_file, 0, NULL, FILE_BEGIN);

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    DWORD sys_gran = sys_info.dwAllocationGranularity;

    DWORD exe_data_segment_map_start = (exe_data_segment_start / sys_gran) * sys_gran;
    DWORD exe_data_segment_map_offset = exe_data_segment_start - exe_data_segment_map_start;

    HANDLE exe_map_file = CreateFileMappingW(exe_file,
                                             NULL,
                                             PAGE_READONLY,
                                             0,
                                             0,
                                             NULL);
    if (exe_map_file == NULL) {
        log_error("Windows error: {}", windows_get_last_error_utf8());
        std::abort();
    }

    LPVOID exe_map_addr = MapViewOfFile(exe_map_file,
                                        FILE_MAP_READ,
                                        0,
                                        exe_data_segment_map_start,
                                        0);
    if (exe_map_addr == NULL) {
        log_error("Windows error: {}", windows_get_last_error_utf8());
        std::abort();
    }

    LPVOID exe_data_segment_addr = reinterpret_cast<BYTE*>(exe_map_addr) + exe_data_segment_map_offset;
    assert(reinterpret_cast<BYTE*>(exe_data_segment_addr)[0] == 'P');
    assert(reinterpret_cast<BYTE*>(exe_data_segment_addr)[1] == 'K');
    assert(reinterpret_cast<BYTE*>(exe_data_segment_addr)[2] == '\3');
    assert(reinterpret_cast<BYTE*>(exe_data_segment_addr)[3] == '\4');
    if (!PHYSFS_mountMemory(exe_data_segment_addr, exe_data_segment_length, NULL, "assets.zip", "/", false)) {
        log_error("Failed to mount PhysFS from memory-mapped file.");
        log_error("PhysFS error: {}", PHYSFS_getLastError());
        std::abort();
    }
#endif
#else
    // Add search paths for PhysFS
    const char* search_paths[] = {
            "assets", "assets.zip", "../assets.zip"
    };
    for (const char* search_path : search_paths) {
        if (PHYSFS_mount(search_path, "/", false)) break;
    }

#endif

    // Initialize resource pools
    res = std::make_unique<Resources>();

    // Initialize Squirrel VM
    sqvm = std::make_unique<VM>();
    sqvm->init(this);

    pre_init();
}

void Engine::base_init() {
    ZoneScoped

    sg_desc g_desc = {.context = sapp_sgcontext()};
    sg_setup(g_desc);

    simgui_desc_t imgui_desc = {.ini_filename = "imgui.ini"};
    simgui_setup(&imgui_desc);

    sgp_desc gp_desc = {};
    sgp_setup(&gp_desc);

    sg_imgui_init(&sg_imgui);

    clear_pass_action = sg_pass_action {
        .colors = {
            {
                .action = SG_ACTION_CLEAR,
                .value = {0.0f, 0.0f, 0.0f, 1.0f}
            }
        },
    };

    stm_setup();
    last_frame_time = stm_now();
    for (int i = 0; i < past_measured_dts.size(); i++) {
        past_measured_dts[i] = sapp_frame_duration();
    }

    // Initialize input
    input = std::make_unique<Input>();

    // Initialize sound
    sound = std::make_unique<Sound>();

    // Initialize sprite renderer
    // Initialize tilemap renderer
    sprite_renderer = std::make_unique<SpriteRenderer>();
    sprite_renderer->init(this);

    // Initialize camera
    camera = std::make_unique<Camera>(game_width, game_height);

    collision_manager = std::make_unique<CollisionManager>(32);

    // Register APIs
    register_api();

    init();
}

void Engine::base_release() {
    ZoneScoped

    release();

    while (!scene_stack.empty()) {
        auto& scene = scene_stack.back();
        scene.release();
        scene_stack.pop_back();
    }

    sprite_renderer.release();

    res->release_with_label(make_res_label("default"));
    res->release_with_label(make_res_label("all"));

    sg_imgui_discard(&sg_imgui);
    simgui_shutdown();
    sg_shutdown();

    log_info("Quitting game!");
    log_release();

    PHYSFS_deinit();

    sqvm.release();
}

void Engine::base_update(double dt) {
    ZoneScoped

    uint64_t new_time = stm_now();
    measured_dt = stm_sec(stm_diff(new_time, last_frame_time));
    measured_fps = int(round(1.0 / measured_dt));
    last_frame_time = new_time;
    for (int i = 0; i < past_measured_dts.size()-1; i++) {
        past_measured_dts[i] = past_measured_dts[i+1];
    }
    past_measured_dts[past_measured_dts.size()-1] = measured_dt;
    measured_avg_dt = 0.0;
    for (int i = 0; i < past_measured_dts.size(); i++) {
        measured_avg_dt += past_measured_dts[i];
    }
    measured_avg_dt /= past_measured_dts.size();
    measured_avg_fps = int(round(1.0 / measured_avg_dt));

    if (needs_reload) {
        scene_stack.back().reload();
        needs_reload = false;
    }

    const int width = sapp_width(), height = sapp_height();
    float ratio = width/(float)height;
    sgp_begin(width, height);
    sgp_viewport(0, 0, width, height);
    // sgp_project(-ratio, ratio, 1.0f, -1.0f);

    update(dt);

    if (!scene_stack.empty()) {
        auto& scene = scene_stack.back();
        scene.update(dt);
    }

    res->foreach<Text>([dt](Text& text) {
        text.update(dt);
    });
    res->foreach<Animation>([dt](Animation& anim) {
        anim.update(dt);
    });
    res->scriptable_update(dt);

    collision_manager->update();

    input->after_update();

    simgui_new_frame({ width, height, sapp_frame_duration(), sapp_dpi_scale() });

    if (show_debug_menu) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("sokol-gfx")) {
                ImGui::MenuItem("Buffers", 0, &sg_imgui.buffers.open);
                ImGui::MenuItem("Images", 0, &sg_imgui.images.open);
                ImGui::MenuItem("Shaders", 0, &sg_imgui.shaders.open);
                ImGui::MenuItem("Pipelines", 0, &sg_imgui.pipelines.open);
                ImGui::MenuItem("Passes", 0, &sg_imgui.passes.open);
                ImGui::MenuItem("Calls", 0, &sg_imgui.capture.open);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    sg_imgui_draw(&sg_imgui);
}

void Engine::base_event(const sapp_event* ev) {
    ZoneScoped

    static double time_since_esc_pressed = 0.0;
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN && ev->key_code == SAPP_KEYCODE_ESCAPE) {
        time_since_esc_pressed += sapp_frame_duration();
    }
    else if (ev->type == SAPP_EVENTTYPE_KEY_DOWN && ev->key_code == SAPP_KEYCODE_ESCAPE) {
        if (time_since_esc_pressed >= 1.0) { sapp_quit(); }
        time_since_esc_pressed = 0.0;
    }

    /*
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN && ev->key_code == SAPP_KEYCODE_F10) {
        window_size_multiplier = 1 + (window_size_multiplier % 4);
        window_width = window_size_multiplier * game_width;
        window_height = window_size_multiplier * game_height;
        SDL_SetWindowSize(window, window_width, window_height);
        viewport_rect = irect(0, 0, window_width, window_height);
    }
     */

    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN && ev->key_code == SAPP_KEYCODE_F9) {
        show_debug_menu = !show_debug_menu;
    }
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN && ev->key_code == SAPP_KEYCODE_F11) {
        sapp_toggle_fullscreen();
        if (sapp_is_fullscreen()) {
            int window_width = sapp_width();
            int window_height = sapp_height();
            int ideal_window_width = window_height * game_width / game_height;
            int letterbox = (window_width - ideal_window_width)/2;
            viewport_rect = irect(letterbox, 0, ideal_window_width, window_height);
        }
        else {
            int window_width = sapp_width();
            int window_height = sapp_height();
            // window_width = window_size_multiplier * game_width;
            // window_height = window_size_multiplier * game_height;
            // SDL_SetWindowSize(window, window_width, window_height);
            viewport_rect = irect(0, 0, window_width, window_height);
        }
    }

    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN && ev->key_code == SAPP_KEYCODE_F5) {
        needs_reload = true;
    }

    input->key_event(ev);

    simgui_handle_event(ev);
}

void Engine::base_render() {
    ZoneScoped

    sg_begin_default_pass(clear_pass_action, sapp_width(), sapp_height());

    if (!scene_stack.empty()) {
        auto& scene = scene_stack.back();
        scene.render();
    }
    sprite_renderer->draw(this);
    collision_manager->debug_render();

    res->scriptable_render();

    sgp_flush();
    sgp_end();

    simgui_render();

    sg_end_pass();
    sg_commit();
}

void Engine::reset() {
    ZoneScoped

    collision_manager = std::make_unique<CollisionManager>();

    sqvm = std::make_unique<VM>();
    sqvm->init(this);

    register_api();
}
