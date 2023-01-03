//
// Created by lasagnaphil on 2021-07-31.
//

#ifndef THESYSTEM_FILE_H
#define THESYSTEM_FILE_H

#include <vector>
#include <string>

std::string strip_relative_path(std::string_view path);

std::vector<char> load_file_to_buffer(std::string_view filename);

std::string get_parent_dir(const std::string& path);

#endif //THESYSTEM_FILE_H
