//
// Created by lasagnaphil on 8/14/2021.
//

#ifndef THESYSTEM_ARENA_ALLOCATOR_H
#define THESYSTEM_ARENA_ALLOCATOR_H

#include <string>
#include <cstddef>
#include <cstdlib>

inline void* aligned_alloc(std::size_t size, std::size_t alignment){
    if(alignment < alignof(void*)) {
        alignment = alignof(void*);
    }
    std::size_t space = size + alignment - 1;
    void* allocated_mem = ::operator new(space + sizeof(void*));
    void* aligned_mem = static_cast<void*>(static_cast<char*>(allocated_mem) + sizeof(void*));
    std::align(alignment, size, aligned_mem, space);
    *(static_cast<void**>(aligned_mem) - 1) = allocated_mem;
    return aligned_mem;
}

inline void aligned_free(void* p) noexcept {
    ::operator delete(*(static_cast<void**>(p) - 1));
}

class arena_allocator
{
    uint8_t* data;
    size_t capacity;
    size_t size;
    const char* name;

public:
    explicit arena_allocator(size_t capacity, const char* name = NULL) : capacity(capacity), name(name) {
        data = (uint8_t*)aligned_alloc(capacity, alignof(void*));
    }
    ~arena_allocator() {
        aligned_free(data);
    }
    arena_allocator(const arena_allocator& other, const char* name = NULL) :
        data((uint8_t*)aligned_alloc(other.capacity, alignof(void*))),
        capacity(other.capacity), size(other.size), name(name) {

        std::memcpy(data, other.data, other.capacity);
    }

    arena_allocator& operator=(const arena_allocator& other) {
        using std::swap;
        arena_allocator copy(other);
        swap(*this, copy);
        return *this;
    }

    arena_allocator(arena_allocator&& other) noexcept {
        swap(*this, other);
    }

    friend void swap(arena_allocator& lhs, arena_allocator& rhs) noexcept {
        std::swap(lhs.data, rhs.data);
        std::swap(lhs.size, rhs.size);
        std::swap(lhs.capacity, rhs.capacity);
        std::swap(lhs.name, rhs.name);
    }

    void* allocate(size_t n, int flags = 0) {
        void* ptr = data + size;
        size += n;
        return ptr;
    }

    void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0) {
        size = ((size - 1 - offset) / alignment + 1) * alignment + offset;
        void* ptr = data + size;
        size += n;
        return ptr;
    }

    void deallocate(void* p, size_t n) {
        // Do nothing
    }

    const char* get_name() const { return name; }
    void        set_name(const char* name) { this->name = name; }
};

inline bool operator==(const arena_allocator&, const arena_allocator&) { return true;  }
inline bool operator!=(const arena_allocator&, const arena_allocator&) { return false; }

#endif //THESYSTEM_ARENA_ALLOCATOR_H
