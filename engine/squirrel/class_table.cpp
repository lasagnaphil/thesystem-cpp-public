//
// Created by lasagnaphil on 10/11/2021.
//

#include "class_table.h"

#include "core/log.h"

sq::Class ClassTable::get_class(uint32_t tid) const {
    auto it = data.find(tid);
    if (it != data.end()) {
        return it->second.cls;
    } else {
        log_error("Cannot find class of typeid {}!", tid);
        return sq::Class();
    }
}

void* ClassTable::get_constructor_ptr(uint32_t tid) const {
    auto it = data.find(tid);
    if (it != data.end()) {
        return it->second.ctor.get();
    } else {
        log_error("Cannot find constructor of class with typeid {}!", tid);
        return nullptr;
    }
}
