//
// Created by lasagnaphil on 2021-08-01.
//

#ifndef THESYSTEM_IMAGE_H
#define THESYSTEM_IMAGE_H

#include "core/reflect.h"

#include <string>

CLASS(Resource) Image {
    std::string filename;
    int width, height, num_channels_in_file, num_channels;
    unsigned char* data = nullptr;

public:
    Image() = default;
    // ~Image() { release(); }

    void load_from_file(const std::string& filename, int num_channels = 0);

    FUNCTION(getter)
    int get_width() { return width; }
    FUNCTION(getter)
    int get_height() { return height; }
    FUNCTION(getter)
    int get_num_channels_in_file() { return num_channels_in_file; }
    FUNCTION(getter)
    int get_num_channels() { return num_channels; }

    unsigned char* get_data() { return data; }

    void release();
};

#endif //THESYSTEM_IMAGE_H
