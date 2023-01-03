//
// Created by lasagnaphil on 10/11/2021.
//

#ifndef THESYSTEM_CLASS_TABLE_H
#define THESYSTEM_CLASS_TABLE_H

#include "resource_pool.h"
#include "squirrel/object.h"

#include <memory>
#include <parallel_hashmap/phmap.h>
#include <functional>

class ClassTable {
public:
    struct Entry {
        sq::Class cls;
        std::unique_ptr<void, void(*)(void const*)> ctor;
    };

    template <class T, class... Args>
    void add_class(uint32_t tid, sq::Class cls, std::function<T*(Args...)>* ctor_ptr) {
        auto ctor_uniq_ptr = std::unique_ptr<void, void(*)(void const*)>(reinterpret_cast<void*>(ctor_ptr), [](void const* ptr) {
            auto p = reinterpret_cast<const std::function<T*(Args...)>*>(ptr);
            delete p;
        });
        data.insert({tid, Entry{cls, std::move(ctor_uniq_ptr)}});
    }

    template <class T, class... Args>
    void add_class(uint32_t tid, sq::Class cls, T*(*ctor_ptr)(Args...)) {
        auto ctor_uniq_ptr = std::unique_ptr<void, void(*)(void const*)>(reinterpret_cast<void*>(ctor_ptr), &ClassTable::do_nothing);
        data.insert({tid, Entry{cls, std::move(ctor_uniq_ptr)}});
    }

    template <class T, class... Args>
    void add_class(uint32_t tid, sq::Class cls, Ref<T>(*ctor_ptr)(Args...)) {
        auto ctor_uniq_ptr = std::unique_ptr<void, void(*)(void const*)>(reinterpret_cast<void*>(ctor_ptr), &ClassTable::do_nothing);
        data.insert({tid, Entry{cls, std::move(ctor_uniq_ptr)}});
    }

    void add_class(uint32_t tid, sq::Class cls) {
        auto ctor_uniq_ptr = std::unique_ptr<void, void(*)(void const*)>(nullptr, &ClassTable::do_nothing);
        data.insert({tid, Entry{cls, std::move(ctor_uniq_ptr)}});
    }

    sq::Class get_class(uint32_t tid) const;

    template <class T>
    sq::Class get_class() const {
        auto it = data.find(type_id<T>());
        if (it != data.end()) {
            return it->second.cls;
        } else {
            log_error("Cannot find class of type {}!", type_name<T>());
            return sq::Class();
        }
    }

    void* get_constructor_ptr(uint32_t tid) const;

private:
    static void do_nothing(void const* data) {};

    phmap::flat_hash_map<uint32_t, Entry> data;
};

#endif //THESYSTEM_CLASS_TABLE_H
