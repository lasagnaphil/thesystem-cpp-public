#include "virtual_alloc.h"

#include "core/log.h"

// Some code taken from vmcontainer library:
// Copyright Miro Knejp 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <sys/mman.h>
#  include <unistd.h>
#endif

#include <cassert>

#include "core/windows_utils.h"

void* virtual_alloc(std::size_t num_bytes)
{
    assert(num_bytes > 0);

#ifdef _WIN32
    auto const offset = ::VirtualAlloc(nullptr, num_bytes, MEM_RESERVE, PAGE_NOACCESS);
    if(offset == nullptr)
    {
        log_error("virtual_alloc() fail: {}", windows_get_last_error_utf8());
        return nullptr;
    }
    return offset;
#else
    auto const offset = ::mmap(nullptr, num_bytes, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0);
    if(offset == MAP_FAILED)
    {
      return nullptr;
    }
    return offset;
#endif
}

void virtual_free(void* offset, std::size_t num_bytes)
{
#ifdef _WIN32
    auto const result = ::VirtualFree(offset, 0, MEM_RELEASE);
    (void)result;
    assert(result != 0);
#else
    auto const result = ::munmap(offset, num_bytes);
    (void)result;
    assert(result == 0);
#endif
}

void virtual_commit(void* offset, std::size_t num_bytes)
{
    assert(num_bytes > 0);

#ifdef _WIN32
    auto const result = ::VirtualAlloc(offset, num_bytes, MEM_COMMIT, PAGE_READWRITE);
    if(result == nullptr)
    {
        log_error("virtual_commit() fail: {}", windows_get_last_error_utf8());
        std::abort();
    }
#else
    auto const result = ::mprotect(offset, num_bytes, PROT_READ | PROT_WRITE);
    if(result != 0)
    {
        std::abort();
    }
#endif
}

void virtual_decommit(void* offset, std::size_t num_bytes)
{
#ifdef _WIN32
    auto const result = ::VirtualFree(offset, num_bytes, MEM_DECOMMIT);
    (void)result;
    assert(result != 0);
#else
    auto const result1 = ::madvise(offset, num_bytes, MADV_DONTNEED);
    (void)result1;
    assert(result1 == 0);
    auto const result2 = ::mprotect(offset, num_bytes, PROT_NONE);
    (void)result2;
    assert(result2 == 0);
#endif
}

std::size_t virtual_page_size = []{
#ifdef _WIN32
    auto info = SYSTEM_INFO{};
    ::GetSystemInfo(&info);
    return static_cast<std::size_t>(info.dwPageSize);
#else
    static_cast<std::size_t>(::getpagesize());
#endif
}();
