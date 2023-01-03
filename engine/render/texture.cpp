//
// Created by lasagnaphil on 2021-08-01.
//

#include "texture.h"

#include "image.h"
#include "engine.h"
#include "resources.h"
#include "core/log.h"

Ref<Texture> Texture::from_image(std::string filename) {
    return from_image_file(filename);
}

Ref<Texture> Texture::from_image_file(const std::string& filename,
                                      int num_channels, int min_filter, int mag_filter, int wrap_u, int wrap_v) {
    Image image;
    image.load_from_file(filename, num_channels);
    auto tex = from_bytes(image.get_data(), image.get_width(), image.get_height(),
                          num_channels, min_filter, mag_filter, wrap_u, wrap_v);
    image.release();
    return tex;
}

Ref<Texture> Texture::from_bytes(const uint8_t* bytes, int width, int height,
                                 int num_channels, int min_filter, int mag_filter, int wrap_u, int wrap_v) {
    auto res = Engine::instance().get_resources();

    auto desc = sg_image_desc{
        .width = width,
        .height = height,
        .min_filter = sg_filter(min_filter),
        .mag_filter = sg_filter(mag_filter),
        .wrap_u = sg_wrap(wrap_u),
        .wrap_v = sg_wrap(wrap_v),
    };
    if (num_channels == 1) {
        desc.pixel_format = SG_PIXELFORMAT_R8;
    }
    else if (num_channels == 4) {
        desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    }
    else {
        log_error("Unsupported number of channels = {}!", num_channels);
        return {};
    }
    if (bytes) {
        desc.data.subimage[0][0] = {bytes, size_t(width * height * num_channels)};
    }
    auto ref = res->new_item<Texture>();
    ref.get()->img = sg_make_image(desc);
    return ref;
}

ivec2 Texture::get_size() {
    auto info = sg_query_image_info(img);
    return {info.width, info.height};
}
