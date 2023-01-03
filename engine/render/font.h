//
// Created by lasagnaphil on 9/30/2021.
//

#ifndef THESYSTEM_FONT_H
#define THESYSTEM_FONT_H

#include "core/reflect.h"
#include "render/texture.h"
#include "parallel_hashmap/phmap.h"
#include "squirrel/object.h"

struct FontCharInfo {
    uint32_t id;
    uint16_t x, y, width, height;
    int16_t xoffset, yoffset;
    uint16_t xadvance;
    uint16_t page;
    uint16_t chnl;
};

enum class FontImageChannelType : uint8_t {
    GlyphData = 0, Outline = 1, GlyphAndOutline = 2, Zero = 3, One = 4
};

CLASS(Resource) Font {
public:
    std::string face;
    uint16_t size;

    bool bold;
    bool italic;
    bool unicode;
    bool smooth;
    bool aa;

    std::string charset;
    uint16_t stretchH;
    uint16_t padding[4];
    uint16_t spacing[2];
    uint16_t outline;

    uint16_t line_height;
    uint16_t base;
    uint16_t scale_w;
    uint16_t scale_h;

    bool packed;
    FontImageChannelType channel_types[4];

    std::vector<Ref<Texture>> pages;
    std::vector<FontCharInfo> chars;
    phmap::flat_hash_map<uint32_t, uint32_t> char_map;

    static Ref<Font> load_xml(std::string bmfile);
};

#endif //THESYSTEM_FONT_H
