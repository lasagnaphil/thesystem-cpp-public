//
// Created by lasagnaphil on 2021-07-31.
//

#include "file.h"
#include "physfs.h"
#include "core/log.h"
#include <string>

std::string strip_relative_path(std::string_view path) {
    std::string path_str = std::string(path);
    if (path_str.find("../") == 0) {
        path_str = path_str.substr(3);
    }
    return path_str;
}

std::vector<char> load_file_to_buffer(std::string_view filename) {
    std::string path = strip_relative_path(filename);
    PHYSFS_file* file = PHYSFS_openRead(path.c_str());
    if (file == nullptr) {
        log_error("Failed to load file {}!", filename);
        return {};
    }
    PHYSFS_sint64 file_size = PHYSFS_fileLength(file);
    std::vector<char> buf(file_size);
    PHYSFS_read(file, buf.data(), 1, file_size);
    PHYSFS_close(file);
    return buf;
}

std::string get_parent_dir(const std::string &path) {
    int idx = path.rfind("/");
    if (idx != std::string::npos) {
        return path.substr(0, idx);
    }
    else {
        log_error("Failed to get relative path of {}", path);
        return "";
    }
}

