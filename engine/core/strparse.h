//
// Created by lasagnaphil on 2021-07-31.
//

#ifndef THESYSTEM_STRPARSE_H
#define THESYSTEM_STRPARSE_H

#include <string>

uint32_t strbuf_to_uint32(const char* buf, size_t len) {
    uint32_t n = 0;
    while (len--) {
        n = 10*n + *buf++ - '0';
    }
    return n;
}

int32_t strbuf_to_int32(const char* buf, size_t len) {
    int32_t n = 0;
    int32_t sign = 1;

    if (len) {
        switch (*buf) {
            case '-': sign = -1;
            case '+': --len, ++buf;
        }
    }

    while (len-- && isdigit(*buf)) {
        n = 10*n + *buf++ - '0';
    }
    return n*sign;
}

#endif //THESYSTEM_STRPARSE_H
