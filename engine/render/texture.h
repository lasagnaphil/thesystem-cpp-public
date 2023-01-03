//
// Created by lasagnaphil on 2021-08-01.
//

#ifndef THESYSTEM_TEXTURE_H
#define THESYSTEM_TEXTURE_H

#include <sokol_gfx.h>
#include "resource_pool.h"
#include "core/types.h"
#include "core/reflect.h"

class Engine;

CLASS(Resource) Texture {
public:
    static Ref<Texture> from_image(std::string filename);
    static Ref<Texture> from_image_file(const std::string& filename,
                                    int num_channels = 4,
                                    int min_filter = SG_FILTER_NEAREST,
                                    int mag_filter = SG_FILTER_NEAREST,
                                    int wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                                    int wrap_v = SG_WRAP_CLAMP_TO_EDGE);
    static Ref<Texture> from_bytes(const uint8_t* bytes, int width, int height,
                                   int num_channels = 4,
                                   int min_filter = SG_FILTER_NEAREST,
                                   int mag_filter = SG_FILTER_NEAREST,
                                   int wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                                   int wrap_v = SG_WRAP_CLAMP_TO_EDGE);

    ivec2 get_size();

    sg_image img;
};

#endif //THESYSTEM_TEXTURE_H
