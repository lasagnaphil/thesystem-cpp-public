//
// Created by lasagnaphil on 10/11/2021.
//

#ifndef THESYSTEM_OBJECTS_H
#define THESYSTEM_OBJECTS_H

#include "squirrel.h"
#include "core/log.h"

namespace sq {

struct Object {
    HSQOBJECT obj;
    Object() { sq_resetobject(&obj); }
    Object(HSQOBJECT obj) : obj(obj) {}

    template <class T>
    T cast() const {
        auto casted = T(obj);
        log_assert(casted.type_check(), "Typecheck failed!");
        return casted;
    }
    bool type_check() { return true; }

    template <class T>
    T unsafe_cast() const { return T(obj); }

    bool is_null() const { return sq_isnull(obj); }
    void reset() { sq_resetobject(&obj); }
};

struct Function : public Object {
    Function() : Object() {}
    Function(HSQOBJECT obj) : Object(obj) {
        log_assert(type_check(), "Typecheck failed!");
    }
    bool type_check() { return sq_isclosure(obj); }
};

struct Table : public Object {
    Table() : Object() {}
    Table(HSQOBJECT obj) : Object(obj) {
        log_assert(type_check(), "Typecheck failed!");
    }
    bool type_check() { return sq_istable(obj); }
};

struct Class : public Object {
    Class() : Object() {}
    Class(HSQOBJECT obj) : Object(obj) {
        log_assert(type_check(), "Typecheck failed!");
    }
    bool type_check() { return sq_isclass(obj); }
};

struct Array : public Object {
    Array() : Object() {}
    Array(HSQOBJECT obj) : Object(obj) {
        log_assert(type_check(), "Typecheck failed!");
    }
    bool type_check() { return sq_isarray(obj); }
};

struct Instance : public Object {
    Instance() : Object() {}
    Instance(HSQOBJECT obj) : Object(obj) {
        log_assert(type_check(), "Typecheck failed!");
    }
    bool type_check() { return sq_isinstance(obj); }
};

struct Module : public Table {
    Module() : Table() {}
    Module(HSQOBJECT obj) : Table(obj) {
        log_assert(type_check(), "Typecheck failed!");
    }
    bool type_check() { return sq_istable(obj); }
};

struct Enum : public Table {
    Enum() : Table() {}
    Enum(HSQOBJECT obj) : Table(obj) {
        log_assert(type_check(), "Typecheck failed!");
    }
    bool type_check() { return sq_istable(obj); }
};

}

#endif //THESYSTEM_OBJECTS_H
