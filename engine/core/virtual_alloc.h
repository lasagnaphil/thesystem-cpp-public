/* Cross-platform virtual alloc. */

#pragma once

#include <cstddef>

void* virtual_alloc(std::size_t num_bytes);

void virtual_free(void* offset, std::size_t num_bytes);

void virtual_commit(void* offset, std::size_t num_bytes);

void virtual_decommit(void* offset, std::size_t num_bytes);

extern std::size_t virtual_page_size;