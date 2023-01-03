#pragma once

#include <vector>
#include <string>
#include <cassert>

#include "core/xxhash.h"
#include "core/log.h"
#include "core/virtual_alloc.h"

#include "reflection.h"

using ResourceLabel = XXH32_hash_t;

inline ResourceLabel make_res_label(const char* c_str) {
    return XXH32(c_str, strlen(c_str), 0);
}

inline ResourceLabel make_res_label(const std::string& str) {
    return XXH32(str.data(), str.size(), 0);
}

template <class T>
class ResourcePool;

// "Safe" pointer that includes generational index in unused 16 bits from 64-bit address space.
template <class T>
struct Ref {
    uintptr_t addr;

    Ref() : addr(0) {}
    Ref(uintptr_t addr) : addr(addr) {}
    Ref(const T* ptr, uint16_t gen) {
        addr = reinterpret_cast<uintptr_t>(ptr) | ((uintptr_t)gen << UINT64_C(48));
    }

    explicit operator bool() const {
        return addr != 0;
    }

    inline uint16_t generation() const { return (uint16_t)(addr >> UINT64_C(48)); }
    // TODO: check if this is standards-compliant in x86-64
    inline T* get_unsafe() const { return reinterpret_cast<T*>((addr << UINT64_C(16)) >> UINT64_C(16)); }
    inline T* get() const;
    bool check() const;

    uint32_t get_type_id() const;

    template <class U>
    bool inherits_type() const {
        return typeinfo<U>::descendant_lookup_table[get_type_id()];
    }

    template <class U>
    inline Ref<U> cast_unsafe() const { return Ref<U>(addr); }

    inline bool operator==(Ref other) const { return addr == other.addr; }
    inline bool operator!=(Ref other) const { return addr != other.addr; }
};

template <>
struct Ref<void> {
    uintptr_t addr;

    Ref() : addr(0) {}
    Ref(uintptr_t addr) : addr(addr) {}
    Ref(const void* ptr, uint16_t gen) {
        addr = reinterpret_cast<uintptr_t>(ptr) | ((uintptr_t)gen << UINT64_C(48));
    }
    template <class T>
    Ref(Ref<T> ref) : addr(ref.addr) {}

    explicit operator bool() const {
        return addr != 0;
    }

    inline uint16_t generation() const { return (uint16_t)(addr >> UINT64_C(48)); }
    // TODO: check if this is standards-compliant in x86-64
    template <class T>
    inline T* get_unsafe() const { return reinterpret_cast<T*>((addr << UINT64_C(16)) >> UINT64_C(16)); }
    template <class T>
    inline T* get() const;
    template <class T>
    bool check() const;

    uint32_t get_type_id() const;

    template <class U>
    bool inherits_type() const {
        return typeinfo<U>::descendant_lookup_table[get_type_id()];
    }

    template <class T>
    inline Ref<T> cast_unsafe() const {
        return Ref<T>(addr);
    }

    inline bool operator==(Ref other) const { return addr == other.addr; }
    inline bool operator!=(Ref other) const { return addr != other.addr; }
};

using AnyRef = Ref<void>;

template <class T>
struct std::hash<Ref<T>>
{
    std::size_t operator()(const Ref<T>& ref) const {
        return std::hash<uintptr_t>()(ref.addr);
    }
};

template <class T>
class StableResourcePool {
public:

    // 8-byte metadata header
    struct Metadata {
        uint32_t index_or_resource_label = 0;
        uint16_t generation = 1;
        uint8_t tid = type_id<T>();
        bool occupied = false;

        inline void set_occupied(ResourceLabel label) {
            index_or_resource_label = label;
            occupied = true;
        }

        inline void set_free() {
            index_or_resource_label = 0xFFFFFFFF;
            generation++;
            occupied = false;
        }

        inline void change_index(uint32_t free_list_index) {
            assert(!occupied);
            index_or_resource_label = free_list_index;
        }
    };

    // Metadata header should always be 8 bytes!
    static_assert(sizeof(Metadata) == 8);

    // Node that includes data along with metadata header
    struct Node {
        Metadata meta;
        T item;
    };

private:
    // mknejp::vmcontainer::pinned_vector<Node> nodes;
    Node* nodes = nullptr;
    uint32_t count = 0;
    uint32_t max_count = 0;
    uint32_t reserved_bytes = 0;
    uint32_t committed_bytes = 0;

    ResourceLabel cur_resource_label;

    uint32_t free_list_front = 0xFFFFFFFF;
    uint32_t free_list_back = 0xFFFFFFFF;

    uint32_t round_bytes_to_page_alignment(uint32_t bytes) {
        return ((bytes - 1) / virtual_page_size + 1) * virtual_page_size;
    }

    void expand_memory(uint32_t new_bytes) {
        if (new_bytes > reserved_bytes) {
            log_error("Inserted too many items in ResourcePool<{}>!", type_name<T>());
            log_error("reserved_bytes={}, count={}", reserved_bytes, count);
        }
        else if (new_bytes > committed_bytes) {
            virtual_commit((unsigned char*)nodes + committed_bytes, new_bytes - committed_bytes);
        }
        committed_bytes = new_bytes;
    }

public:
    StableResourcePool(uint32_t max_elements = 1048576)
    {
        reserved_bytes = round_bytes_to_page_alignment(sizeof(Node) * max_elements);
        nodes = static_cast<Node*>(virtual_alloc(reserved_bytes));
    }
    ~StableResourcePool() {
        virtual_free(nodes, reserved_bytes);
    }

    uint32_t size() const { return count; }

    void clear() {
        for (int i = 0; i < max_count; i++) {
            auto& node = nodes[i];
            if (node.meta.occupied) {
                node.item.~T();
            }
        }

        count = 0;
        max_count = 0;

        virtual_decommit(nodes, committed_bytes);
        committed_bytes = 0;

        cur_resource_label = make_res_label("all");
        free_list_front = free_list_back = 0xFFFFFFFF;
    }

    ResourceLabel get_resource_label() {
        return this->cur_resource_label;
    }

    void set_resource_label(ResourceLabel label) {
        this->cur_resource_label = label;
    }

    template <class ...Args>
    Ref<T> new_item(Args&&... args) {
        Ref<T> ptr;
        if (free_list_front == 0xFFFFFFFF) {
            // If free list is empty, create new free list node
            assert(max_count == count);
            uint32_t new_bytes = round_bytes_to_page_alignment(sizeof(Node)*(max_count + 1));
            expand_memory(new_bytes);
            auto& node = nodes[max_count];
            new (&node.meta) Metadata();
            node.meta.set_occupied(cur_resource_label);
            new (&node.item) T(args...);
            ptr = Ref<T>(&node.item, node.meta.generation);
            max_count++;
        }
        else {
            // Take the front node of free list
            Node& node = nodes[free_list_front];
            // Remove node from free list
            free_list_front = node.meta.index_or_resource_label;
            if (free_list_front == 0xFFFFFFFF) {
                free_list_back = 0xFFFFFFFF;
            }
            node.meta.set_occupied(cur_resource_label);
            new (&node.item) T(args...);
            ptr = Ref<T>(&node.item, node.meta.generation);
        }
        count++;
        return ptr;
    }

    Ref<T> insert(const T& item) {
        auto ptr = new_item();
        *ptr.get_unsafe() = item;
        return ptr;
    }

    Ref<T> insert(T&& item) {
        auto ptr = new_item();
        *ptr.get_unsafe() = std::move(item);
        return ptr;
    }

    // TODO: more optimized implementation?
    /*
    void insert_n(std::span<Id<T>> ids, const T& item) {
        items.reserve(items.size() + ids.size());
        resource_labels.reserve(items.size() + ids.size());
        dense_to_sparse_map.reserve(dense_to_sparse_map.size() + ids.size());
        fragmented = true;

        for (int i = 0; i < ids.size(); i++) {
            ids[i] = insert(item);
        }
    }
     */

    // WARNING: Extremely evil but necessary pointer manipulation to access metadata header
    static inline Node* get_node_from_item(const T* item) {
        return reinterpret_cast<Node*>(reinterpret_cast<std::uintptr_t>(item) - offsetof(Node, item));
    }

    inline uint32_t get_index(Ref<T> ref) {
        T* item = ref.get_unsafe();
        Node* node = get_node_from_item(item);

        ptrdiff_t index = node - nodes;
        uint16_t gen = ref.generation();
        assert(index >= 0 && index < max_count);

        Metadata& meta = node->meta;
        assert(meta.generation == gen);

        return (uint32_t)index;
    }

    bool release(Ref<T> ref) {
        T* item = ref.get_unsafe();
        Node* node = get_node_from_item(item);

        uint32_t index = node - nodes;
        uint16_t gen = ref.generation();
        assert(index >= 0 && index < max_count);

        assert(node->meta.generation == gen);
        assert(node->meta.tid == type_id<T>());

        node->item.~T();
        node->meta.set_free();

        // If free list is empty, create free list with one element
        if (free_list_front == 0xFFFFFFFF) {
            free_list_back = free_list_front = index;
        }
        else {
            nodes[free_list_back].meta.change_index(index);
            free_list_back = index;
        }
        nodes[free_list_back].meta.change_index(0xFFFFFFFF);

        count--;

        return true;
    }

    int release(ResourceLabel label) {
        int released_count = 0;
        for (int index = 0; index < max_count; index++) {
            auto& node = nodes[index];
            if (node.meta.occupied && node.meta.index_or_resource_label == label) {
                node.item.~T();
                node.meta.set_free();

                if (free_list_front == 0xFFFFFFFF) {
                    free_list_back = free_list_front = index;
                }
                else {
                    nodes[free_list_back].meta.change_index(index);
                    free_list_back = index;
                }
                nodes[free_list_back].meta.change_index(0xFFFFFFFF);

                released_count++;
            }
        }
        count -= released_count;
        return released_count;
    }

    Ref<T> clone(Ref<T> ref) {
        assert(is_valid(ref));
        T* item = ref.get_unsafe();
        Node* node = get_node_from_item(item);
        uint32_t index = node - nodes;
        Ref<T> new_ref = insert(T());
        T* new_item = get(new_ref);
        Node* new_node = get_node_from_item(new_item);
        new_node->item = nodes[index].item;
        return new_ref;
    }

    bool is_valid(Ref<T> ref) const {
        T* item = ref.get_unsafe();
        Node* node = get_node_from_item(item);
        ptrdiff_t index = node - nodes;
        uint16_t gen = ref.generation();
        if (index < 0 || index >= max_count) return false;
        return node->meta.occupied && node->meta.generation == gen;
    }

    const T* get(Ref<T> ref) const {
        T* item = ref.get_unsafe();
        Node* node = get_node_from_item(item);
        ptrdiff_t index = node - nodes;
        uint16_t gen = ref.generation();
        assert(index >= 0 || index < max_count);
        assert(node->meta.occupied && node->meta.generation == gen);
        return item;
    }

    T* get(Ref<T> ref) {
        T* item = ref.get_unsafe();
        Node* node = get_node_from_item(item);
        ptrdiff_t index = node - nodes;
        uint16_t gen = ref.generation();
        assert(index >= 0 || index < max_count);
        assert(node->meta.occupied && node->meta.generation == gen);
        return item;
    }

    const T* try_get(Ref<T> ref) const {
        T* item = ref.get_unsafe();
        Node* node = get_node_from_item(item);
        ptrdiff_t index = node - nodes;
        uint16_t gen = ref.generation();
        if (index < 0 || index >= max_count) return nullptr;
        if (!node->meta.occupied || node->meta.generation != gen) return nullptr;
        return item;
    }

    T* try_get(Ref<T> ref) {
        T* item = ref.get_unsafe();
        Node* node = get_node_from_item(item);
        ptrdiff_t index = node - nodes;
        uint16_t gen = ref.generation();
        if (index < 0 || index >= max_count) return nullptr;
        if (!node->meta.occupied || node->meta.generation != gen) return nullptr;
        return item;
    }

    template <class Fun>
    void foreach(Fun&& fun) {
        for (int i = 0; i < max_count; i++) {
            auto& node = nodes[i];
            if (node.meta.occupied) {
                fun(node.item);
            }
        }
    }

    template <class Fun>
    void foreach_ref(Fun&& fun) {
        for (int i = 0; i < max_count; i++) {
            auto& node = nodes[i];
            if (node.meta.occupied) {
                Ref<T> ref(&node.item, node.meta.generation);
                fun(ref, node.item);
            }
        }
    }

    template <class TBase, class Fun>
    void foreach_ref(Fun&& fun) {
        for (int i = 0; i < max_count; i++) {
            auto& node = nodes[i];
            if (node.meta.occupied) {
                Ref<TBase> ref(static_cast<TBase*>(&node.item), node.meta.generation);
                fun(ref, static_cast<TBase&>(node.item));
            }
        }
    }
};

template<class T>
T *Ref<T>::get() const {
    T* item = get_unsafe();
    auto node = StableResourcePool<T>::get_node_from_item(item);
    uint16_t gen = generation();
    assert(node->meta.occupied && node->meta.generation == gen);
    return item;
}

template<class T>
bool Ref<T>::check() const {
    T* item = get_unsafe();
    auto node = StableResourcePool<T>::get_node_from_item(item);
    uint16_t gen = generation();
    return node->meta.occupied && node->meta.generation == gen;
}

template<class T>
uint32_t Ref<T>::get_type_id() const {
    T* item = get_unsafe();
    auto node = StableResourcePool<T>::get_node_from_item(item);
    return node->meta.tid;
}

template<class T>
T *Ref<void>::get() const {
    T* item = get_unsafe<T>();
    auto node = StableResourcePool<T>::get_node_from_item(item);
    uint16_t gen = generation();
    assert(node->meta.occupied && node->meta.generation == gen &&
        typeinfo<T>::descendant_lookup_table[node->meta.tid]);
    return item;
}

template<class T>
bool Ref<void>::check() const {
    T* item = get_unsafe<T>();
    auto node = StableResourcePool<T>::get_node_from_item(item);
    uint16_t gen = generation();
    return node->meta.occupied && node->meta.generation == gen &&
        typeinfo<T>::descendant_lookup_table[node->meta.tid];
}
