//
// Created by lasagnaphil on 2021-08-01.
//

#include "image.h"
#include "core/log.h"
#include "core/file.h"
#include "stb/stb_image.h"

#include "resources.h"

void Image::load_from_file(const std::string& filename, int num_channels) {
    this->filename = filename;
    std::vector<char> buf = load_file_to_buffer(filename.c_str());
    if (buf.empty()) return;
    this->data = stbi_load_from_memory((stbi_uc*)buf.data(), buf.size(), &this->width, &this->height, &this->num_channels_in_file, num_channels);
    if (!this->data) {
        log_error("Failed to load image {}!", filename.c_str());
    }
    this->num_channels = num_channels;
}

void Image::release() {
    if (data) {
        stbi_image_free(data);
        data = nullptr;
    }
}

