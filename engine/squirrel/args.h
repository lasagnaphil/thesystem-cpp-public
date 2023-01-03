//
// Created by lasagnaphil on 10/11/2021.
//

#ifndef THESYSTEM_ARGS_H
#define THESYSTEM_ARGS_H

#include "resources.h"

#include "core/log.h"
#include "core/types.h"
#include "core/rect.h"
#include "squirrel/object.h"
#include "squirrel/class_table.h"
#include "squirrel/function_pool.h"
#include "squirrel/utils.h"

#include <string>

#define SQ_EXPECT(res, ...) if (SQ_FAILED(res)) { log_error(__VA_ARGS__); return SQ_ERROR; }

class Engine;

namespace sq {

template<typename T>
struct is_ref
{
    static constexpr bool value = false;
    using inner_type = void;
};

template<template<typename...> class C, typename U>
struct is_ref<C<U>>
{
    static constexpr bool value =
            std::is_same<C<U>,Ref<U>>::value;
    using inner_type = U;
};

ClassTable& get_class_table(HSQUIRRELVM vm);
FunctionPool& get_function_pool(HSQUIRRELVM vm);

template <class T>
inline sq::Class get_class(HSQUIRRELVM vm) {
    return get_class_table(vm).get_class<T>();
}

inline sq::Class get_class(HSQUIRRELVM vm, uint32_t tid) {
    return get_class_table(vm).get_class(tid);
}

template <class T>
static SQInteger class_destructor(SQUserPointer ptr, SQInteger size) {
    T* p = static_cast<T*>(ptr);
    delete p;
    return 0;
}

template<class T>
static SQInteger class_ptr_destructor(SQUserPointer ptr, SQInteger size) {
    T** p = static_cast<T**>(ptr);
    delete *p;
    return 0;
}

template <class T>
inline SQInteger get_pointer(HSQUIRRELVM vm, SQInteger index, T*& value) {
    if constexpr (is_resource_v<T>) {
        SQUserPointer ptr;
        if (SQ_FAILED(sq_getinstanceup(vm, index, &ptr, nullptr))) {
            log_error("Failed to get instance of class {} from stack", type_name<typename std::remove_pointer<T>::type>());
            return SQ_ERROR;
        }
        auto ref = Ref<T>(reinterpret_cast<uintptr_t>(ptr));
        value = ref.get();
        return SQ_OK;
    }
    else {
        SQObjectType type = sq_gettype(vm, index);
        if (type == OT_USERPOINTER) {
            SQUserPointer val;
            if (SQ_FAILED(sq_getuserpointer(vm, index, &val))) {
                log_error("Could not get instance of class {} from squirrel stack", type_name<typename std::remove_pointer<T>::type>());
                return SQ_ERROR;
            }
            value = reinterpret_cast<T*>(val);
            return SQ_OK;
        }
        else if (type == OT_INSTANCE) {
            SQUserPointer val;
            if (SQ_FAILED(sq_getinstanceup(vm, index, &val, nullptr))) {
                log_error("Could not get instance of class {} from squirrel stack", type_name<typename std::remove_pointer<T>::type>());
                return SQ_ERROR;
            }
            value = reinterpret_cast<T*>(val);
            return SQ_OK;
        }
        else {
            log_error("Could not get instance of class {} from squirrel stack", type_name<typename std::remove_pointer<T>::type>());
            return SQ_ERROR;
        }
    }
}

template <class T>
inline SQInteger get_ref(HSQUIRRELVM vm, SQInteger index, Ref<T>& ref) {
    SQUserPointer ptr;
    if (SQ_FAILED(sq_getinstanceup(vm, index, &ptr, nullptr))) {
        log_error("Failed to get instance of class {} from stack", type_name<typename std::remove_pointer<T>::type>(), index);
        return SQ_ERROR;
    }
    ref.addr = reinterpret_cast<uintptr_t>(ptr);
    return SQ_OK;
}

inline SQInteger get_str(HSQUIRRELVM vm, SQInteger index, const char* str) {
    SQ_EXPECT(sq_getstring(vm, index, &str), "Could not get string from stack");
    return SQ_OK;
}

template <class T, typename = std::enable_if_t<!is_ref<T>::value>>
inline SQInteger get_value(HSQUIRRELVM vm, SQInteger index, T& value){
    SQUserPointer ptr;
    SQObjectType type = sq_gettype(vm, index);
    if (type == OT_USERDATA) {
        if (SQ_FAILED(sq_getuserdata(vm, index, &ptr, nullptr))) {
            log_error("Could not get instance of class {} from stack", type_name<T>());
            return SQ_ERROR;
        }
        T** p = reinterpret_cast<T**>(ptr);
        value = **p;
        return SQ_OK;
    }
    else if (type == OT_INSTANCE) {
        if (SQ_FAILED(sq_getinstanceup(vm, index, &ptr, nullptr))) {
            log_error("Could not get instance of class {} from stack", type_name<T>());
            return SQ_ERROR;
        }
        T* p = reinterpret_cast<T*>(ptr);
        value = *p;
        return SQ_OK;
    }
    else {
        log_error("Could not get instance of class {} from stack", type_name<T>());
        return SQ_ERROR;
    }
}

template<>
inline SQInteger get_value(HSQUIRRELVM vm, SQInteger index, Object& value){
    SQ_EXPECT(sq_getstackobj(vm, index, &value.obj), "Could not get Object from squirrel stack")
    sq_addref(vm, &value.obj);
    return SQ_OK;
}

template<>
inline SQInteger get_value(HSQUIRRELVM vm, SQInteger index, Table& value){
    SQ_EXPECT(sq_getstackobj(vm, index, &value.obj), "Could not get Table from squirrel stack")
    sq_addref(vm, &value.obj);
    return SQ_OK;
}

#define GET_VALUE_INTEGER(TYPE) \
template<> \
inline SQInteger get_value(HSQUIRRELVM vm, SQInteger index, TYPE& value) { \
    SQInteger i; \
    SQ_EXPECT(sq_getinteger(vm, index, &i), "Could not get char from squirrel stack"); \
    value = static_cast<TYPE>(i); \
    return SQ_OK; \
}

GET_VALUE_INTEGER(char)
GET_VALUE_INTEGER(short)
GET_VALUE_INTEGER(int)
GET_VALUE_INTEGER(long)
GET_VALUE_INTEGER(long long)
GET_VALUE_INTEGER(unsigned char)
GET_VALUE_INTEGER(unsigned short)
GET_VALUE_INTEGER(unsigned int)
GET_VALUE_INTEGER(unsigned long)
GET_VALUE_INTEGER(unsigned long long)

#undef GET_VALUE_INTEGER

template<>
inline SQInteger get_value(HSQUIRRELVM vm, SQInteger index, float& value){
    SQ_EXPECT(sq_getfloat(vm, index, &value), "Could not get float from squirrel stack")
    return SQ_OK;
}

template<>
inline SQInteger get_value(HSQUIRRELVM vm, SQInteger index, bool& value){
    SQBool val;
    SQ_EXPECT(sq_getbool(vm, index, &val), "Could not get bool from squirrel stack")
    value = static_cast<bool>(val);
    return SQ_OK;
}

template<>
inline SQInteger get_value(HSQUIRRELVM vm, SQInteger index, std::string& value) {
    const SQChar* val;
    SQ_EXPECT(sq_getstring(vm, index, &val), "Could not get string from squirrel stack");
    if (val == nullptr) value = "";
    else {
        SQInteger len = sq_getsize(vm,index);
        value = std::string(val, len);
    }
    return SQ_OK;
}

template<>
inline SQInteger get_value(HSQUIRRELVM vm, SQInteger index, vec2& value) {
    assert(index >= 0);
    SQ_EXPECT(sq_gettype(vm, index) == OT_ARRAY, "Type vec2 should be array")

    SQInteger top = sq_gettop(vm);

    sq_pushinteger(vm, 0);
    SQ_EXPECT(sq_get(vm, index), "Failed to get elem from vec2 array!");
    SQ_EXPECT(sq_getfloat(vm, -1, &value.x), "Element in vec2 array is not number")
    sq_pushinteger(vm, 1);
    SQ_EXPECT(sq_get(vm, index), "Failed to get elem from vec2 array!");
    SQ_EXPECT(sq_getfloat(vm, -1, &value.y), "Element in vec2 array is not number");

    sq_settop(vm, top);

    return SQ_OK;
}

template<>
inline SQInteger get_value(HSQUIRRELVM vm, SQInteger index, ivec2& value) {
    assert(index >= 0);
    SQ_EXPECT(sq_gettype(vm, index) == OT_ARRAY, "Type ivec2 should be array");

    SQInteger top = sq_gettop(vm);

    SQInteger x, y;
    sq_pushinteger(vm, 0);
    SQ_EXPECT(sq_get(vm, index), "Failed to get elem from ivec2 array!");
    SQ_EXPECT(sq_getinteger(vm, -1, &x), "Element in ivec2 array is not number");
    sq_pushinteger(vm, 1);
    SQ_EXPECT(sq_get(vm, index), "Failed to get elem from ivec2 array");
    SQ_EXPECT(sq_getinteger(vm, -1, &y), "Element in ivec2 array is not number");
    value.x = x; value.y = y;

    sq_settop(vm, top);

    return SQ_OK;
}

template<>
inline SQInteger get_value(HSQUIRRELVM vm, SQInteger index, rect& value) {
    assert(index >= 0);
    SQ_EXPECT(sq_gettype(vm, index) == OT_ARRAY, "Type rect should be array");

    SQInteger top = sq_gettop(vm);

    sq_pushinteger(vm, 0);
    sq_get(vm, index);
    SQ_EXPECT(sq_getfloat(vm, -1, &value.pos.x), "Element in rect array is not number");
    sq_pushinteger(vm, 1);
    sq_get(vm, index);
    SQ_EXPECT(sq_getfloat(vm, -1, &value.pos.y), "Element in rect array is not number");
    sq_pushinteger(vm, 2);
    sq_get(vm, index);
    SQ_EXPECT(sq_getfloat(vm, -1, &value.size.x), "Element in rect array is not number");
    sq_pushinteger(vm, 3);
    sq_get(vm, index-1);
    SQ_EXPECT(sq_getfloat(vm, -1, &value.size.y), "Element in rect array is not number");

    sq_settop(vm, top);

    return SQ_OK;
}

template<>
inline SQInteger get_value(HSQUIRRELVM vm, SQInteger index, irect& value) {
    assert(index >= 0);
    SQ_EXPECT(sq_gettype(vm, index) == OT_ARRAY, "Type irect should be array");

    SQInteger top = sq_gettop(vm);

    SQInteger x, y, sx, sy;

    sq_pushinteger(vm, 0);
    sq_get(vm, index);
    SQ_EXPECT(sq_getinteger(vm, -1, &x), "Element in irect array is not number");
    sq_pushinteger(vm, 1);
    sq_get(vm, index);
    SQ_EXPECT(sq_getinteger(vm, -1, &y), "Element in irect array is not number");
    sq_pushinteger(vm, 2);
    sq_get(vm, index);
    SQ_EXPECT(sq_getinteger(vm, -1, &sx), "Element in irect array is not number");
    sq_pushinteger(vm, 3);
    sq_get(vm, index);
    SQ_EXPECT(sq_getinteger(vm, -1, &sy), "Element in irect array is not number");
    value.pos.x = x; value.pos.y = y; value.size.x = sx; value.size.y = sy;

    sq_settop(vm, top);

    return SQ_OK;
}

template <class T>
inline SQInteger get(HSQUIRRELVM vm, SQInteger index,
                     T& value) {
    if constexpr (is_ref<T>::value) {
        return get_ref(vm, index, value);
    }
    else if constexpr (std::is_pointer<T>::value) {
        return get_pointer<typename std::remove_pointer<T>::type>(vm, index, value);
    }
    else if constexpr (std::is_same_v<T, const char*>) {
        return get_str(vm, index, value);
    }
    else {
        return get_value<typename std::remove_cv<T>::type>(vm, index, value);
    }
}

template <class T>
inline typename std::remove_cv<T>::type get_return(HSQUIRRELVM vm, SQInteger index, SQInteger& error) {
    typename std::remove_cv<T>::type value = {};
    error = get(vm, index, value);
    return value;
}

template <class T>
inline SQInteger pop(HSQUIRRELVM vm, T& value) {
    SQInteger err = get(vm, -1, value);
    sq_pop(vm, -1);
    return err;
}

template <class T>
inline void push_ref(HSQUIRRELVM vm, const Ref<T>& ref) {
    static_assert(is_resource_v<T>);
    constexpr uint32_t tid = type_id<T>();
    Class cls = get_class(vm, tid);
    assert(!cls.is_null());
    sq_pushobject(vm, cls.obj);
    sq_createinstance(vm, -1);
    sq_remove(vm, -2);
    sq_setinstanceup(vm, -1, reinterpret_cast<SQUserPointer>(ref.addr));
    sq_settypetag(vm, -1, reinterpret_cast<SQUserPointer>(tid));
}

/*
template <class T>
inline void push_value(HSQUIRRELVM vm, const T& value) {
    const uint32_t tid = type_id<T>();
    Class cls = get_class(vm, tid);
    if (cls.is_null()) {
        T** data = reinterpret_cast<T**>(sq_newuserdata(vm, sizeof(T*)));
        *data = new T(value);
        sq_setreleasehook(vm, -1, class_ptr_destructor<T>);
        sq_settypetag(vm, -1, reinterpret_cast<SQUserPointer>(tid));
    }
    else {
        sq_pushobject(vm, cls.obj);
        sq_createinstance(vm, -1);
        sq_remove(vm, -2);
        sq_setinstanceup(vm, -1, reinterpret_cast<SQUserPointer>(new T(value)));
        sq_settypetag(vm, -1, reinterpret_cast<SQUserPointer>(tid));
        sq_setreleasehook(vm, -1, class_destructor<T>);
    }
}
 */

inline void push_value(HSQUIRRELVM vm, const HSQOBJECT& obj) {
    sq_pushobject(vm, obj);
}

inline void push_value(HSQUIRRELVM vm, const Object& value) {
    sq_pushobject(vm, value.obj);
}

inline void push_value(HSQUIRRELVM vm, const bool& value) {
    sq_pushbool(vm, value);
}

inline void push_value(HSQUIRRELVM vm, const int& value) {
    sq_pushinteger(vm, value);
}

inline void push_value(HSQUIRRELVM vm, const short& value) {
    sq_pushinteger(vm, value);
}

inline void push_value(HSQUIRRELVM vm, const long& value) {
    sq_pushinteger(vm, value);
}

inline void push_value(HSQUIRRELVM vm, const char& value) {
    sq_pushinteger(vm, value);
}

inline void push_value(HSQUIRRELVM vm, const signed char& value) {
    sq_pushinteger(vm, value);
}

inline void push_value(HSQUIRRELVM vm, const unsigned int& value) {
    sq_pushinteger(vm, *reinterpret_cast<const int*>(&value));
}

inline void push_value(HSQUIRRELVM vm, const unsigned short& value) {
    sq_pushinteger(vm, *reinterpret_cast<const short*>(&value));
}

inline void push_value(HSQUIRRELVM vm, const unsigned long& value) {
    sq_pushinteger(vm, *reinterpret_cast<const long*>(&value));
}

inline void push_value(HSQUIRRELVM vm, const unsigned char& value) {
    sq_pushinteger(vm, *reinterpret_cast<const char*>(&value));
}

inline void push_value(HSQUIRRELVM vm, const unsigned long long& value) {
    sq_pushinteger(vm, *reinterpret_cast<const long long*>(&value));
}

inline void push_value(HSQUIRRELVM vm, const long long& value) {
    sq_pushinteger(vm, *reinterpret_cast<const long long*>(&value));
}

inline void push_value(HSQUIRRELVM vm, const float& value) {
    sq_pushfloat(vm, value);
}

inline void push_value(HSQUIRRELVM vm, const std::string& value) {
    sq_pushstring(vm, value.c_str(), value.size());
}

inline void push_value(HSQUIRRELVM vm, const vec2& value) {
    sq_newarray(vm, 2);
    sq_pushinteger(vm, 0);
    sq_pushfloat(vm, value.x);
    sq_rawset(vm, -3);
    sq_pushinteger(vm, 1);
    sq_pushfloat(vm, value.y);
    sq_rawset(vm, -3);
}

inline void push_value(HSQUIRRELVM vm, const ivec2& value) {
    sq_newarray(vm, 2);
    sq_pushinteger(vm, 0);
    sq_pushinteger(vm, value.x);
    sq_rawset(vm, -3);
    sq_pushinteger(vm, 1);
    sq_pushinteger(vm, value.y);
    sq_rawset(vm, -3);
}

inline void push_value(HSQUIRRELVM vm, const rect& value) {
    sq_newarray(vm, 2);
    sq_pushinteger(vm, 0);
    sq_pushfloat(vm, value.pos.x);
    sq_rawset(vm, -3);
    sq_pushinteger(vm, 1);
    sq_pushfloat(vm, value.pos.y);
    sq_rawset(vm, -3);
    sq_pushinteger(vm, 2);
    sq_pushfloat(vm, value.size.x);
    sq_rawset(vm, -3);
    sq_pushinteger(vm, 3);
    sq_pushfloat(vm, value.size.y);
    sq_rawset(vm, -3);
}

inline void push_value(HSQUIRRELVM vm, const irect& value) {
    sq_newarray(vm, 4);
    sq_pushinteger(vm, 0);
    sq_pushinteger(vm, value.pos.x);
    sq_rawset(vm, -3);
    sq_pushinteger(vm, 1);
    sq_pushinteger(vm, value.pos.y);
    sq_rawset(vm, -3);
    sq_pushinteger(vm, 2);
    sq_pushinteger(vm, value.size.x);
    sq_rawset(vm, -3);
    sq_pushinteger(vm, 3);
    sq_pushinteger(vm, value.size.y);
    sq_rawset(vm, -3);
}

template <class T>
inline void push_by_ptr(HSQUIRRELVM vm, T* value) {
    constexpr uint32_t tid = type_id<T>();
    if (value == nullptr) {
        sq_pushnull(vm);
    }
    else {
        Class cls = get_class(vm, tid);
        if (cls.is_null()) {
            sq_pushuserpointer(vm, (SQUserPointer)(value));
        }
        else {
            sq_pushobject(vm, cls.obj);
            sq_createinstance(vm, -1);
            sq_remove(vm, -2);
            sq_setinstanceup(vm, -1, (SQUserPointer)(value));
            sq_settypetag(vm, -1, reinterpret_cast<SQUserPointer>(tid));
        }
    }
}

template <typename T>
inline void push(HSQUIRRELVM vm, const T& value) {
    if constexpr (std::is_pointer_v<T>) {
        push_by_ptr<typename std::remove_pointer<typename std::remove_cv<T>::type>::type>(vm, value);
    }
    else if constexpr (is_ref<T>::value) {
        push_ref(vm, value);
    }
    else {
        push_value(vm, value);
    }
}

inline void push_args(HSQUIRRELVM vm) { }

template <class First, class... Rest>
inline void push_args(HSQUIRRELVM vm, First&& first, Rest&&... rest) {
    push(vm, first);
    push_args(vm, std::forward<Rest>(rest)...);
}

}

#endif //THESYSTEM_ARGS_H
