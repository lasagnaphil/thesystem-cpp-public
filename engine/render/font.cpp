//
// Created by lasagnaphil on 9/30/2021.
//

#include "font.h"

#include "engine.h"
#include "core/log.h"
#include "core/strutil.h"
#include "core/file.h"
#include "squirrel/vm.h"
#include <pugixml.hpp>

Ref<Font> Font::load_xml(std::string bmfile) {

    auto res = Engine::instance().get_resources();
    auto font_ref = res->new_item<Font>();

    int i = bmfile.find_last_of('/');
    std::string bmfolder = bmfile.substr(0, i+1);

    std::vector<char> buf = load_file_to_buffer(bmfile.c_str());
    if (buf.empty()) return {};
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(buf.data(), buf.size());
    if (!result) {
        log_error("pugixml error in {}: {}", bmfile, result.description());
        return {};
    }

    auto& font = *font_ref.get();
    auto node_font = doc.document_element();

    auto node_info = node_font.child("info");
    font.face = node_info.attribute("face").as_string();
    font.size = node_info.attribute("size").as_int();
    font.bold = node_info.attribute("bold").as_bool();
    font.italic = node_info.attribute("italic").as_bool();
    font.charset = node_info.attribute("charset").as_string();
    font.unicode = node_info.attribute("unicode").as_int();
    font.stretchH = node_info.attribute("stretchH").as_int();
    font.smooth = node_info.attribute("smooth").as_bool();
    font.aa = node_info.attribute("aa").as_bool();
    auto padding_str = node_info.attribute("padding").as_string();
    auto padding_tokens = strutil::split(padding_str, ",");
    assert(padding_tokens.size() == 4);
    font.padding[0] = std::stoi(padding_tokens[0]);
    font.padding[1] = std::stoi(padding_tokens[1]);
    font.padding[2] = std::stoi(padding_tokens[2]);
    font.padding[3] = std::stoi(padding_tokens[3]);
    auto spacing_str = node_info.attribute("spacing").as_string();
    auto spacing_tokens = strutil::split(spacing_str, ",");
    assert(spacing_tokens.size() == 2);
    font.spacing[0] = std::stoi(spacing_tokens[0]);
    font.spacing[1] = std::stoi(spacing_tokens[1]);

    auto node_common = node_font.child("common");
    font.line_height = node_common.attribute("lineHeight").as_int();
    font.base = node_common.attribute("base").as_int();
    font.scale_w = node_common.attribute("scaleW").as_int();
    font.scale_h = node_common.attribute("scaleH").as_int();
    auto num_pages = node_common.attribute("pages").as_int();
    font.packed = node_common.attribute("packed").as_bool();
    font.channel_types[0] = (FontImageChannelType) node_common.attribute("alphaChnl").as_int();
    font.channel_types[1] = (FontImageChannelType) node_common.attribute("redChnl").as_int();
    font.channel_types[2] = (FontImageChannelType) node_common.attribute("greenChnl").as_int();
    font.channel_types[3] = (FontImageChannelType) node_common.attribute("blueChnl").as_int();

    auto node_pages = node_font.child("pages");
    font.pages.reserve(num_pages);
    for (auto node_page : node_pages.children("page")) {
        auto filename = bmfolder + node_page.attribute("file").as_string();
        auto tex_ref = Texture::from_image_file(filename, 4);
        font.pages.push_back(tex_ref);
    }
    assert(num_pages == font.pages.size());

    auto node_chars = node_font.child("chars");
    auto num_chars = node_chars.attribute("count").as_int();
    font.chars.reserve(num_chars);
    for (auto node_char : node_chars.children("char")) {
        FontCharInfo info;
        info.id = node_char.attribute("id").as_int();
        info.x = node_char.attribute("x").as_int();
        info.y = node_char.attribute("y").as_int();
        info.width = node_char.attribute("width").as_int();
        info.height = node_char.attribute("height").as_int();
        info.xoffset = node_char.attribute("xoffset").as_int();
        info.yoffset = node_char.attribute("yoffset").as_int();
        info.xadvance = node_char.attribute("xadvance").as_int();
        info.page = node_char.attribute("page").as_int();
        info.chnl = node_char.attribute("chnl").as_int();
        font.char_map[info.id] = font.chars.size();
        font.chars.push_back(info);
    }
    assert(font.chars.size() == num_chars);

    return font_ref;
}
