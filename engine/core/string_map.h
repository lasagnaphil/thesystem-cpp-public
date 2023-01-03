//
// Created by lasagnaphil on 10/24/2021.
//

#ifndef THESYSTEM_STRING_MAP_H
#define THESYSTEM_STRING_MAP_H

#include <cstdlib>
#include <cstring>
#include <parallel_hashmap/phmap.h>

class string_key {
    char *m_s;

public:
    string_key() { m_s = nullptr; }
    string_key(const char* s) {
        auto len = strlen(s);
        m_s = new char[len + 1];
        memcpy(m_s, s, sizeof(char) * (len+1));
    }
    ~string_key() { delete m_s; }

    bool operator== (const char* s) const { return strcmp(m_s, s) == 0; }
    bool operator== (const string_key& ss) const { return strcmp(m_s, ss.m_s) == 0; }
    friend bool operator== (const char* s, const string_key& ss) { return strcmp(s, ss.m_s) == 0; }
    bool operator< (const char* s) const { return strcmp(m_s, s) < 0; }
    bool operator< (const string_key& ss) const { return strcmp(m_s, ss.m_s) < 0; }
    friend bool operator< (const char* s, const string_key& ss) { return strcmp(s, ss.m_s) < 0; }
    bool operator> (const char* s) const { return strcmp(m_s, s) > 0; }
    bool operator> (const string_key& ss) const { return strcmp(m_s, ss.m_s) > 0; }
    friend bool operator> (const char* s, const string_key& ss) { return strcmp(s, ss.m_s) > 0; }

    const char* c_str() const { return m_s; }
};

namespace std {
    template <>
    struct hash<string_key> {
        std::size_t operator()(const string_key& k) {
            auto s = k.c_str();
            return XXH64(s, strlen(s), 0);
        }
    };
}

// TODO: use string_key instead of std::string to save allocations
template <class V>
using string_map = phmap::parallel_flat_hash_map<std::string, V>;

#endif //THESYSTEM_STRING_MAP_H
