//
// Created by lasagnaphil on 10/12/2021.
//

#ifndef THESYSTEM_FUNCTIONPOOL_H
#define THESYSTEM_FUNCTIONPOOL_H

#include <memory>
#include <parallel_hashmap/phmap.h>
#include <squirrel/object.h>

class FunctionPool {
public:
    template <class FunctionPtr>
    void add_fn(FunctionPtr* fn_ptr) {
        auto ctor_uniq_ptr = std::unique_ptr<void, void(*)(void const*)>(fn_ptr, [](void const* ptr) {
            auto p = reinterpret_cast<const FunctionPtr*>(ptr);
            delete p;
        });
        data.insert(std::move(ctor_uniq_ptr));
    }

private:
    phmap::flat_hash_set<std::unique_ptr<void, void(*)(void const*)>> data;
};

#endif //THESYSTEM_FUNCTIONPOOL_H
