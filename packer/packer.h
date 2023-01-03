#pragma once

#include <render/image.h>

enum PackerResult {
    PACKER_SUCCESS,
    PACKER_ERROR
};

struct Packer {
    static PackerResult compress(const char* dir) {
        return PACKER_ERROR;
    }
    static PackerResult decompress(const char* dir, Resources& res) {
        return PACKER_ERROR;
    }
};