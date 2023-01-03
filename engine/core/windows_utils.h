#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <string>
#include <stringapiset.h>

#include "core/log.h"

std::string windows_utf8_encode(const std::wstring &wstr);

inline std::string windows_get_current_exe_path_utf8();

inline std::wstring windows_get_current_exe_path_utf16();

std::string windows_get_last_error_utf8();

#endif