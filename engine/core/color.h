#pragma once

#include <cstdint>
#include <string_view>
#include <cassert>

#include "core/types.h"
#include "core/log.h"

using rgba = uint32_t;

class Colors {
private:
    static int hex_to_int(char c) {
        int i;
        if (c >= 'a' && c <= 'f') {
            i = c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F') {
            i = c - 'A' + 10;
        }
        else if (c >= '0' && c <= '9') {
            i = c - '0';
        }
        else {
            i = -1;
        }
        return i;
    }
public:
    static rgba from_hex_string(std::string str) {
        return from_hex(std::string_view(str));
    }

    static rgba from_hex(std::string_view str) {
        assert(str[0] == '#');
        assert(str.size() == 7 || str.size() == 9);
        int r1 = hex_to_int(str[1]), r2 = hex_to_int(str[2]);
        int g1 = hex_to_int(str[3]), g2 = hex_to_int(str[4]);
        int b1 = hex_to_int(str[5]), b2 = hex_to_int(str[6]);
        if (r1 == -1 || r2 == -1 || g1 == -1 || g2 == -1 || b1 == -1 || b2 == -1) {
            log_error("Invalid color hex string {}!", str);
            return 0x00000000;
        }
        if (str.size() == 7) {
            return 0xff000000 | r1 | (r2 << 4) | (g1 << 8) | (g2 << 12) | (b1 << 16) | (b2 << 20);
        }
        else {
            int a1 = hex_to_int(str[7]), a2 = hex_to_int(str[8]);
            return r1 | (r2 << 4) | (g1 << 8) | (g2 << 12) | (b1 << 16) | (b2 << 20) | (a1 << 24) | (a2 << 28);
        }
    }
};