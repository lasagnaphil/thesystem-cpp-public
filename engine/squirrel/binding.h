//
// Created by lasagnaphil on 10/10/2021.
//

#ifndef THESYSTEM_BINDING_H
#define THESYSTEM_BINDING_H

#include "engine.h"
#include "core/log.h"

#include "object.h"
#include "args.h"
#include "squirrel/utils.h"
#include "squirrel/scriptable.h"

#include <cstdint>

namespace sq {

template <int... Is>
struct index_list {
};

// Declare primary template for index range builder
template <int MIN, int N, int... Is>
struct range_builder;

// Base step
template <int MIN, int... Is>
struct range_builder<MIN, MIN, Is...> {
    typedef index_list<Is...> type;
};

// Induction step
template <int MIN, int N, int... Is>
struct range_builder : public range_builder<MIN, N - 1, N - 1, Is...> {
};

// Meta-function that returns a [MIN, MAX) index range
template<int MIN, int MAX>
using index_range = typename range_builder<MIN, MAX>::type;

template <typename T> struct Param {static const char type = '.';};

template <> struct Param<char> {static const char type = 'i';};
template <> struct Param<signed char> {static const char type = 'i';};
template <> struct Param<short> {static const char type = 'i';};
template <> struct Param<int> {static const char type = 'i';};
template <> struct Param<long> {static const char type = 'i';};
template <> struct Param<unsigned char> {static const char type = 'i';};
template <> struct Param<unsigned short> {static const char type = 'i';};
template <> struct Param<unsigned int> {static const char type = 'i';};
template <> struct Param<unsigned long> {static const char type = 'i';};
#ifdef _SQ64
template <> struct Param<long long> {static const char type = 'i';};
template <> struct Param<unsigned long long> {static const char type = 'i';};
#endif
template <> struct Param<float> {static const char type = 'n';};
template <> struct Param<double> {static const char type = 'n';};
#ifdef SQUNICODE
template <> struct Param<std::wstring> {static const char type = 's';};
#else
template <> struct Param<std::string> {static const char type = 's';};
#endif
template <> struct Param<Class> {static const char type = 'y';};
template <> struct Param<Function> {static const char type = 'c';};
template <> struct Param<Table> {static const char type = 't';};
template <> struct Param<Array> {static const char type = 'a';};
template <> struct Param<Instance> {static const char type = 'x';};
template <> struct Param<std::nullptr_t> {static const char type = 'o';};

template <typename A>
static void param_packer_type(char* ptr) {
    *ptr = Param<typename std::remove_const<typename std::remove_reference<A>::type>::type>::type;
}

template <typename ...B>
static void param_packer(char* ptr) {
    (param_packer_type<B>(ptr++), ...); // C++17 fold expression
    *ptr = '\0';
}

template <class T, class = std::enable_if_t<!is_resource_v<T>>>
static T* default_class_allocator() {
    return new T;
}

template <class T, class = std::enable_if_t<is_resource_v<T>>>
static Ref<T> default_class_allocator() {
    return Engine::instance().get_resources()->template new_item<T>();
}

template <class T>
static SQInteger default_class_deallocator(SQUserPointer ptr, SQInteger size) {
    if constexpr (is_resource_v<T>) {
        auto ref = Ref<T>(reinterpret_cast<uintptr_t>(ptr));
        if (Engine::instance().get_resources()->template get_pool<T>().release(ref)) return SQ_OK;
        else return SQ_ERROR;
    }
    else {
        delete reinterpret_cast<T*>(ptr);
        return 0;
    }
}

inline bool release(HSQUIRRELVM vm, Object& o) {
    return sq_release(vm, &o.obj);
}

template <class ConstructorPtr, class... Args>
inline void add_class_to_registry(HSQUIRRELVM vm, uint32_t tid, Class cls, ConstructorPtr ctor) {
    auto& class_table = get_class_table(vm);
    class_table.add_class(tid, cls, ctor);
}

inline void add_class_to_registry(HSQUIRRELVM vm, uint32_t tid, Class cls) {
    auto& class_table = get_class_table(vm);
    class_table.add_class(tid, cls);
}

template <class R, class... Args, int... Is>
static R call_constructor_rawfun(HSQUIRRELVM vm, R(*func_ptr)(Args...), index_list<Is...>) {
    SQInteger error;
    R ret = func_ptr(get_return<Args>(vm, Is + 2, error)...);
    if (SQ_FAILED(error)) {
        log_error("Error while calling constructor!");
    }
    return ret;
}

template <class R, class... Args, int... Is>
static R call_constructor_stdfun(HSQUIRRELVM vm, std::function<R(Args...)>* func_ptr, index_list<Is...>) {
    SQInteger error;
    R ret = func_ptr->operator()(get_return<Args>(vm, Is + 2, error)...);
    if (SQ_FAILED(error)) {
        log_error("Error while calling constructor!");
    }
    return ret;
}

template <bool Std, bool Release, class T, class... Args>
static SQInteger class_allocator(HSQUIRRELVM vm) {
    constexpr size_t nparams = sizeof...(Args);
    int off = nparams;

    // stack> cls | params... | up
    if constexpr (is_resource_v<T>) {
        Ref<T> ref;
        if constexpr (Std) {
            std::function<Ref<T>(Args...)>* func_ptr;
            sq_getuserpointer(vm, -1, reinterpret_cast<void**>(&func_ptr));
            ref = call_constructor_stdfun<Ref<T>, Args...>(vm, func_ptr, index_range<0, sizeof...(Args)>());
        }
        else {
            Ref<T>(*func_ptr)(Args...);
            sq_getuserpointer(vm, -1, reinterpret_cast<void**>(&func_ptr));
            ref = call_constructor_rawfun<Ref<T>, Args...>(vm, func_ptr, index_range<0, sizeof...(Args)>());
        }
        if (SQ_FAILED(sq_setinstanceup(vm, -2 - off, reinterpret_cast<SQUserPointer>(ref.addr)))) {
            log_error("Failed to allocate instance of class {}", type_name<T>());
            return SQ_ERROR;
        }
        if constexpr (is_scriptable_v<T>) {
            ref.get()->register_script_methods(-2 - off);
        }
    }
    else {
        T* p;
        if constexpr (Std) {
            std::function<T*(Args...)>* func_ptr;
            sq_getuserpointer(vm, -1, reinterpret_cast<void**>(&func_ptr));
            p = call_constructor_stdfun<T*, Args...>(vm, func_ptr, index_range<0, sizeof...(Args)>());
        }
        else {
            T*(*func_ptr)(Args...);
            sq_getuserpointer(vm, -1, reinterpret_cast<void**>(&func_ptr));
            p = call_constructor_rawfun<T*, Args...>(vm, func_ptr, index_range<0, sizeof...(Args)>());
        }
        if (SQ_FAILED(sq_setinstanceup(vm, -2 - off, p))) {
            log_error("Failed to allocate instance of class {}", type_name<T>());
            return SQ_ERROR;
        }
    }

    if constexpr (Release) {
        sq_setreleasehook(vm, -2 - off, &default_class_deallocator<T>);
    }

    sq_getclass(vm, -2 - off);
    sq_settypetag(vm, -1, reinterpret_cast<SQUserPointer>(type_id<T>()));
    sq_pop(vm, 1);
    return nparams;
}

template <class T, class TBase, bool release, class Ret, class... Args>
inline SQInteger add_class(HSQUIRRELVM vm, Class& cls, Ret(*allocator)(Args...)) {
    if constexpr (is_resource_v<T>) {
        static_assert(std::is_same_v<Ref<T>, Ret>);
    }
    else {
        static_assert(std::is_same_v<T*, Ret>);
    }

    constexpr uint32_t tid = type_id<T>();
    static const size_t nparams = sizeof...(Args);

    if constexpr (std::is_void_v<TBase>) {
        sq_newclass(vm, false);
    }
    else {
        auto& class_table = get_class_table(vm);
        auto base_class = class_table.get_class<TBase>();
        sq_pushobject(vm, base_class.obj);
        sq_newclass(vm, true);
    }
    sq_getstackobj(vm, -1, &cls.obj);
    add_class_to_registry(vm, tid, cls, allocator);
    sq_settypetag(vm, -1, reinterpret_cast<SQUserPointer>(tid));

    sq_pushstring(vm, "constructor", -1);
    sq_pushuserpointer(vm, reinterpret_cast<void*>(allocator));
    static char params[33];
    param_packer<T*, Args...>(params);
    sq_newclosure(vm, &class_allocator<false, release, T, Args...>, 1);
    sq_setparamscheck(vm, nparams + 1, params);
    sq_newslot(vm, -3, false); // Add the constructor method

    return SQ_OK;
}

template <class T>
inline SQInteger add_abstract_class(HSQUIRRELVM vm, Class& cls) {
    constexpr uint32_t tid = type_id<T>();

    sq_newclass(vm, false);
    add_class_to_registry(vm, tid, cls);
    sq_addref(vm, &cls.obj);
    sq_settypetag(vm, -1, reinterpret_cast<SQUserPointer>(tid));
    sq_newslot(vm, -3, false);

    return SQ_OK;
}

template <class R, class... Args, int... Is>
static R call_func_helper_rawptr(HSQUIRRELVM vm, R(*fn)(Args...), index_list<Is...>) {
    static const size_t nparams = sizeof...(Is);
    SQInteger error;
    if constexpr (std::is_void_v<R>) {
        fn(get_return<typename std::remove_reference<Args>::type>(vm, Is + 1, error)...);
        sq_pop(vm, nparams);
        if (SQ_FAILED(error)) {
            sq_throwerror(vm, "Error while calling function");
        }
    }
    else {
        R ret = fn(get_return<typename std::remove_reference<Args>::type>(vm, Is + 1, error)...);
        sq_pop(vm, nparams);
        if (SQ_FAILED(error)) {
            sq_throwerror(vm, "Error while calling function");
        }
        return ret;
    }
}

template <class R, class... Args, int... Is>
static R call_func_helper_stdfun(HSQUIRRELVM vm, std::function<R(Args...)>* fn, index_list<Is...>) {
    static const size_t nparams = sizeof...(Is);
    SQInteger error;
    if constexpr (std::is_void_v<R>) {
        fn->operator()(get_return<typename std::remove_reference<Args>::type>(vm, Is + 1, error)...);
        sq_pop(vm, nparams);
    }
    else {
        R ret = fn->operator()(get_return<typename std::remove_reference<Args>::type>(vm, Is + 1, error)...);
        sq_pop(vm, nparams);
        return ret;
    }
}

template <class R, class T, class... Args, int offset, int... Is>
static R call_member_helper(HSQUIRRELVM vm, std::function<R(T*, Args...)>* fn, index_list<offset, Is...>) {
    static const size_t nparams = sizeof...(Is);
    SQInteger error;
    T* ptr;
    get_pointer(vm, offset + 1, ptr);
    if constexpr (std::is_void_v<R>) {
        fn->operator()(ptr, get_return<typename std::remove_reference<Args>::type>(vm, Is + 1, error)...);
        sq_pop(vm, nparams);
    }
    else {
        R ret = fn->operator()(ptr, get_return<typename std::remove_reference<Args>::type>(vm, Is + 1, error)...);
        sq_pop(vm, nparams);
        return ret;
    }
}

template <int offset, class R, class... Args>
static SQInteger call_func_rawptr(HSQUIRRELVM vm) {
    constexpr size_t nparams = sizeof...(Args);

    R(*fn)(Args...);
    sq_getuserpointer(vm, -1, reinterpret_cast<void**>(&fn));
    if constexpr (std::is_void_v<R>) {
        call_func_helper_rawptr(vm, fn, index_range<offset, nparams + offset>());
        return 0;
    }
    else {
        push(vm, std::forward<R>(call_func_helper_rawptr(vm, fn, index_range<offset, offset + nparams>())));
        return 1;
    }
}

template <int offset, class R, class... Args>
static SQInteger call_func_stdfun(HSQUIRRELVM vm) {
    constexpr size_t nparams = sizeof...(Args);

    std::function<R(Args...)>* fn;
    sq_getuserpointer(vm, -1, reinterpret_cast<void**>(&fn));
    if constexpr (std::is_void_v<R>) {
        call_func_helper_stdfun(vm, fn, index_range<offset, nparams + offset>());
        return 0;
    }
    else {
        push(vm, std::forward<R>(call_func_helper_stdfun(vm, fn, index_range<offset, offset + nparams>())));
        return 1;
    }
}

template <int offset, class R, class T, class... Args>
static SQInteger call_member(HSQUIRRELVM vm) {
    constexpr size_t nparams = sizeof...(Args);

    std::function<R(T*, Args...)>* fn;
    sq_getuserpointer(vm, -1, reinterpret_cast<void**>(&fn));
    if constexpr (std::is_void_v<R>) {
        call_member_helper<R, T, Args...>(vm, fn, index_range<offset, offset + nparams + 1>());
        return 0;
    }
    else {
        push(vm, std::forward<R>(call_member_helper<R, T, Args...>(vm, fn, index_range<offset, offset + nparams + 1>())));
        return 1;
    }
}

template <class R, class... Args>
inline SQInteger add_func(HSQUIRRELVM vm, R(*fn)(Args...), Function& function) {
    constexpr size_t nparams = sizeof...(Args);

    sq_pushuserpointer(vm, reinterpret_cast<void*>(fn));
    static char params[33];
    param_packer<void, Args...>(params);
    sq_newclosure(vm, &call_func_rawptr<1, R, Args...>, 1);
    sq_getstackobj(vm, -1, &function.obj);
    sq_setparamscheck(vm, nparams + 1, params);
    if (SQ_FAILED(sq_newslot(vm, -3, false))) {
        log_error("Failed to bind function");
        return SQ_ERROR;
    }
    return SQ_OK;
}

template <class R, class... Args>
inline SQInteger add_func(HSQUIRRELVM vm, const std::function<R(Args...)>& fn, Function& function) {
    constexpr size_t nparams = sizeof...(Args);

    auto fn_ptr = new std::function<R(Args...)>(fn);
    get_function_pool(vm).add_fn(fn_ptr);
    sq_pushuserpointer(vm, fn_ptr);
    static char params[33];
    param_packer<void, Args...>(params);
    sq_newclosure(vm, &call_func_stdfun<1, R, Args...>, 1);
    sq_getstackobj(vm, -1, &function.obj);
    sq_setparamscheck(vm, nparams + 1, params);
    if (SQ_FAILED(sq_newslot(vm, -3, false))) {
        log_error("Failed to bind function");
        return SQ_ERROR;
    }
    return SQ_OK;
}

// TODO: Two identical add_member_func() overloaded based on member function pointer constness.
//       Maybe there's a better way to refactor this?

template <class T, class R, class... Args>
inline SQInteger add_member_func(HSQUIRRELVM vm, R(T::*fn)(Args...), Function& function) {
    if constexpr (std::is_member_function_pointer<decltype(fn)>::value) {
        constexpr size_t nparams = sizeof...(Args);

        static char params[33];
        param_packer<T*, Args...>(params);

        auto fn_ptr = new std::function<R(T*, Args...)>(std::mem_fn(fn));
        get_function_pool(vm).add_fn(fn_ptr);
        sq_pushuserpointer(vm, fn_ptr);
        sq_newclosure(vm, &call_member<0, R, T, Args...>, 1);

        sq_getstackobj(vm, -1, &function.obj);
        if (SQ_FAILED(sq_setparamscheck(vm, nparams + 1, params))) {
            log_error("Failed to set params check for function");
            return SQ_ERROR;
        }
        if (SQ_FAILED(sq_newslot(vm, -3, true))) {
            log_error("Failed to bind function");
            return SQ_ERROR;
        }
        return SQ_OK;
    }
    else {
        using RawFun = R(*)(Args...);
        RawFun fn_without_class = fn;
        return add_func(vm, fn_without_class, function);
    }
}

template <class T, class R, class... Args>
inline SQInteger add_member_func(HSQUIRRELVM vm, R(T::*fn)(Args...) const, Function& function) {
    if constexpr (std::is_member_function_pointer<decltype(fn)>::value) {
        constexpr size_t nparams = sizeof...(Args);

        static char params[33];
        param_packer<T*, Args...>(params);

        auto fn_ptr = new std::function<R(T*, Args...)>(std::mem_fn(fn));
        get_function_pool(vm).add_fn(fn_ptr);
        sq_pushuserpointer(vm, fn_ptr);
        sq_newclosure(vm, &call_member<0, R, T, Args...>, 1);

        sq_getstackobj(vm, -1, &function.obj);
        sq_setparamscheck(vm, nparams + 1, params);
        if (SQ_FAILED(sq_newslot(vm, -3, true))) {
            log_error("Failed to bind function");
            return SQ_ERROR;
        }
        return SQ_OK;
    }
    else {
        using RawFun = R(*)(Args...);
        RawFun fn_without_class = fn;
        return add_func(vm, fn_without_class, function);
    }
}

inline SQInteger create_instance(HSQUIRRELVM vm, Class cls, Instance& o) {
    auto top = sq_gettop(vm);
    sq_pushobject(vm, cls.obj);
    sq_createinstance(vm, -1);
    sq_getstackobj(vm, -1, &o.obj);
    sq_addref(vm, &o.obj);
    sq_settop(vm, top);
    return SQ_OK;
}

inline SQInteger get_object(HSQUIRRELVM vm, Object o, const char* name, Object& ret) {
    sq_pushobject(vm, o.obj);
    sq_pushstring(vm, name, strlen(name));
    if (SQ_FAILED(sq_get(vm, -2))) {
        sq_pop(vm, 1);
        log_error("Cannot find slot {} in object", name);
        return SQ_ERROR;
    }
    sq_getstackobj(vm, -1, &ret.obj);
    sq_addref(vm, &ret.obj);
    sq_pop(vm, 2);
    return SQ_OK;
}

inline SQInteger get_string(HSQUIRRELVM vm, Object o, const char* name, std::string& ret) {
    sq_pushobject(vm, o.obj);
    sq_pushstring(vm, name, strlen(name));
    if (SQ_FAILED(sq_get(vm, -2))) {
        sq_pop(vm, 1);
        log_error("Cannot find slot {} in object", name);
        return SQ_ERROR;
    }
    const SQChar* c; SQInteger size;
    sq_getstringandsize(vm, -1, &c, &size);
    ret = std::string(c, size);
    sq_pop(vm, 2);
    return SQ_OK;
}

template <class... Args>
inline SQInteger call_return(HSQUIRRELVM vm, Function fun, Object env, Object& ret, Args&&... args) {
    constexpr size_t params = sizeof...(Args);
    SQInteger top = sq_gettop(vm);
    sq_pushobject(vm, fun.obj);
    sq_pushobject(vm, env.obj);
    push_args(vm, std::forward<Args>(args)...);
    if (SQ_FAILED(sq_call(vm, 1 + params, true, true))) {
        log_error("Failed to call function");
        sq_settop(vm, top);
        return SQ_ERROR;
    }
    sq_getstackobj(vm, -1, &ret.obj);
    sq_addref(vm, &ret.obj);
    sq_settop(vm, top);
    return SQ_OK;
}


template <class... Args>
inline SQInteger call_noreturn(HSQUIRRELVM vm, Function fun, Object env, Args&&... args) {
    constexpr size_t params = sizeof...(Args);
    SQInteger top = sq_gettop(vm);
    sq_pushobject(vm, fun.obj);
    sq_pushobject(vm, env.obj);
    push_args(vm, std::forward<Args>(args)...);
    if (SQ_FAILED(sq_call(vm, params + 1, false, true))) {
        log_error("Failed to call function");
        sq_settop(vm, top);
        return SQ_ERROR;
    }
    sq_settop(vm, top);
    return SQ_OK;
}

}
#endif //THESYSTEM_BINDING_H
