//
// Created by lasagnaphil on 9/18/2021.
//

#ifndef THESYSTEM_VM_H
#define THESYSTEM_VM_H

#include <squirrel.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdblob.h>

#include "core/reflect.h"
#include "core/strutil.h"
#include "squirrel/utils.h"
#include "squirrel/binding.h"
#include "squirrel/class_table.h"
#include "squirrel/function_pool.h"

#include "script_module.h"

class Engine;

class VM {
public:
    ~VM();
    void init(Engine* engine);
    void release();

    std::string resolve_file_name(const char* modpath);
    bool check_circular_refs(const char* filename);

    Ref<ScriptModule> require_module(const char* requested_fn, bool must_exist, const char* __name__);

    HSQUIRRELVM handle() { return vm; }

    sq::Module add_module(std::string_view name) {
        sq::Module mod;
        sq_pushroottable(vm);
        sq_pushstring(vm, name.data(), name.size());
        sq_newtable(vm);
        sq_getstackobj(vm, -1, &mod.obj);
        sq_addref(vm, &mod.obj);
        sq_newslot(vm, -3, false);
        sq_pop(vm, 1);
        return mod;
    }

    template <class T, class TBase = base_type_v<T>, class AllocatorFun = decltype(sq::default_class_allocator<T>), bool release = !is_resource_v<T>>
    sq::Class add_class(std::string_view name = type_name<T>(),
                        AllocatorFun allocator = sq::default_class_allocator<T>) {
        static_assert(std::is_void_v<TBase> || (is_resource_v<T> == is_resource_v<TBase>));

        sq::Class cls;
        sq_pushroottable(vm);
        sq_pushstring(vm, name.data(), name.size());
        if (sq::add_class<T, TBase, release>(vm, cls, allocator) < 0) {
            log_error("Failed to add class {}!", name);
        }
        sq_newslot(vm, -3, false); // Add the class
        sq_pop(vm, 1);
        return cls;
    }

    template <class T>
    sq::Class add_abstract_class(std::string_view name) {
        sq::Class cls;
        sq_pushroottable(vm);
        sq_pushstring(vm, name.data(), name.size());
        sq::add_abstract_class<T>(vm, cls);
        sq_newslot(vm, -3, false);
        sq_pop(vm, 1);
        return cls;
    }

    template <class Fun>
    sq::Function add_func(std::string_view name, Fun&& fn) {
        sq::Function function;
        sq_pushroottable(vm);
        sq_pushstring(vm, name.data(), name.size());
        sq::add_func(vm, std::forward<Fun>(fn), function);
        sq_newslot(vm, -3, false);
        sq_pop(vm, 1);
        return function;
    }

    template <class Fun>
    sq::Function add_func(sq::Table table, std::string_view name, Fun&& fn) {
        sq::Function function;
        sq_pushobject(vm, table.obj);
        sq_pushstring(vm, name.data(), name.size());
        sq::add_func(vm, std::forward<Fun>(fn), function);
        sq_newslot(vm, -3, false);
        sq_pop(vm, 1);
        return function;
    }

    template <class Fun>
    sq::Function add_method(sq::Class cls, std::string_view name, Fun&& fn) {
        sq::Function function;
        sq_pushobject(vm, cls.obj);
        sq_pushstring(vm, name.data(), name.size());
        if (SQ_FAILED(sq::add_member_func(vm, std::forward<Fun>(fn), function))) {
            log_error("Failed to add method {}!", name);
        }
        sq_newslot(vm, -3, false);
        sq_pop(vm, 1);
        return function;
    }

    template <class T>
    void add_const(std::string_view name, const T& value) {
        sq_pushconsttable(vm);
        sq_pushstring(vm, name.data(), name.size());
        sq::push<T>(vm, value);
        sq_newslot(vm, -3, false);
        sq_pop(vm, 1);
    }

    template <class T>
    void add_global(std::string_view name, const T& value) {
        sq_pushroottable(vm);
        sq_pushstring(vm, name.data(), name.size());
        sq::push<T>(vm, value);
        sq_newslot(vm, -3, false);
        sq_pop(vm, 1);
    }

    sq::Enum add_enum(std::string_view name) {
        sq::Enum em;
        sq_pushconsttable(vm);
        sq_pushstring(vm, name.data(), name.size());
        sq_newtable(vm);
        sq_getstackobj(vm, -1, &em.obj);
        sq_addref(vm, &em.obj);
        sq_newslot(vm, -3, false);
        sq_pop(vm, 1);
        return em;
    }

    sq::Enum add_enum(std::string_view name, std::initializer_list<std::pair<const char*, SQInteger>> list) {
        sq::Enum em;
        sq_pushconsttable(vm);
        sq_pushstring(vm, name.data(), name.size());
        sq_newtable(vm);
        sq_getstackobj(vm, -1, &em.obj);
        sq_addref(vm, &em.obj);
        for (auto& [enname, enidx] : list) {
            sq_pushstring(vm, enname, strlen(enname));
            sq_pushinteger(vm, enidx);
            sq_newslot(vm, -3, false);
        }
        sq_newslot(vm, -3, false);
        sq_pop(vm, 1);
        return em;
    }

    sq::Table new_table() {
        sq::Table table;
        sq_newtable(vm);
        sq_getstackobj(vm, -1, &table.obj);
        sq_addref(vm, &table.obj);
        sq_poptop(vm);
        return table;
    }

    template <class... Args>
    sq::Instance new_instance(sq::Class cls, Args&&... args) {
        sq::Instance inst = new_instance_without_ctor(cls);
        sq::Function ctor = get<sq::Function>(cls, "constructor");
        call_func(ctor, inst, std::forward<Args>(args)...);
        return inst;
    }

    sq::Instance new_instance_without_ctor(sq::Class cls) {
        sq::Instance inst;
        sq_pushobject(vm, cls.obj);
        sq_createinstance(vm, -1);
        sq_getstackobj(vm, -1, &inst.obj);
        sq_addref(vm, &inst.obj);
        sq_pop(vm, 2);
        return inst;
    }

    template <class T>
    sq::Instance new_instance_from_ptr(T* value) {
        sq::Instance inst;
        sq::push_by_ptr(vm, value);
        sq_getstackobj(vm, -1, &inst.obj);
        sq_addref(vm, &inst.obj);
        sq_pop(vm, 1);
        return inst;
    }

    template <class... Args>
    sq::Object call_func(sq::Function func, sq::Object env, Args&&... args) {
        static const size_t nparams = sizeof...(Args);

        auto top = sq_gettop(vm);
        sq_pushobject(vm, func.obj);
        sq_pushobject(vm, env.obj);
        sq::push_args(vm, std::forward<Args>(args)...);

        sq::Object ret;

        if (SQ_FAILED(sq_call(vm, 1 + nparams, true, true))) {
            sq_settop(vm, top);
            log_error("Failed to call function!");
            return ret;
        }
        sq_getstackobj(vm, -1, &ret.obj);
        sq_addref(vm, &ret.obj);
        sq_settop(vm, top);
        return ret;
    }

    template <class T>
    T get(const sq::Object& o, std::string_view name) {
        SQInteger top = sq_gettop(vm);
        sq_pushobject(vm, o.obj);
        sq_pushstring(vm, name.data(), name.size());
        if (SQ_FAILED(sq_get(vm, top+1))) {
            log_error("Failed to get slot {} from object.", name);
        }
        if constexpr (std::is_base_of_v<sq::Object, T>) {
            HSQOBJECT obj;
            if (SQ_FAILED(sq_getstackobj(vm, top+2, &obj))) {
                log_error("Failed to get slot {} from object.", name);
            }
            sq_settop(vm, top);
            return T(obj);
        }
        else {
            T value;
            if (SQ_FAILED(sq::get<T>(vm, top+2, value))) {
                log_error("Failed to get slot {} from object.", name);
            }
            sq_settop(vm, top);
            return value;
        }
    }

    template <class T>
    T get_or_default(const sq::Object& o, std::string_view name, const T& default_value) {
        SQInteger top = sq_gettop(vm);
        sq_pushobject(vm, o.obj);
        sq_pushstring(vm, name.data(), name.size());
        if (SQ_FAILED(sq_get(vm, top+1))) {
            sq_settop(vm, top);
            return default_value;
        }
        T value;
        if (SQ_FAILED(sq::get<T>(vm, top+2, value))) {
            log_error("Failed to get slot {} from object.", name);
        }
        sq_settop(vm, top);
        return value;
    }

    template <class T>
    bool try_get(const sq::Object& o, std::string_view name, T& value) {
        SQInteger top = sq_gettop(vm);
        sq_pushobject(vm, o.obj);
        sq_pushstring(vm, name.data(), name.size());
        if (SQ_FAILED(sq_get(vm, top+1))) {
            sq_settop(vm, top);
            return false;
        }
        if (SQ_FAILED(sq::get<T>(vm, top+2, value))) {
            log_error("Failed to get slot {} from object.", name);
            return false;
        }
        sq_settop(vm, top);
        return true;
    }

    template <class T>
    void setslot(const sq::Object& object, std::string_view name, const T& value) {
        SQInteger top = sq_gettop(vm);
        sq_pushobject(vm, object.obj);
        sq_pushstring(vm, name.data(), name.size());
        sq::push<T>(vm, value);
        sq_newslot(vm, top+1, false);
        sq_settop(vm, top);
    }

    template <class T>
    void set(const sq::Object& object, std::string_view name, const T& value) {
        SQInteger top = sq_gettop(vm);
        sq_pushobject(vm, object.obj);
        sq_pushstring(vm, name.data(), name.size());
        sq::push<T>(vm, value);
        sq_set(vm, top+1);
        sq_settop(vm, top);
    }

    template <class T>
    Ref<T> get_cppref_unsafe(const sq::Instance& inst) {
        AnyRef ref;
        void* ptr;
        SQInteger top = sq_gettop(vm);
        sq_pushobject(vm, inst.obj);
        sq_getinstanceup(vm, top+1, &ptr, nullptr);
        ref.addr = (uintptr_t)ptr;
        sq_settop(vm, top);
        if (ref.check<T>()) {
            return ref.cast_unsafe<T>();
        }
        else {
            return Ref<T>();
        }
    }

    template <class T>
    bool try_get_cppref(const sq::Instance& inst, Ref<T>& cppref) {
        AnyRef ref;
        void* ptr;
        SQInteger top = sq_gettop(vm);
        sq_pushobject(vm, inst.obj);
        sq_getinstanceup(vm, top+1, &ptr, nullptr);
        ref.addr = (uintptr_t)ptr;
        sq_settop(vm, top);
        if (!ref.check<T>()) return false;
        cppref = ref.cast_unsafe<T>();
        return true;
    }

    std::string to_string(sq::Object obj, bool multiline = true, int indent = 4) {
        return sqx_to_string(vm, obj.obj, multiline, indent);
    }

    ClassTable& get_class_table() {
        return cls_table;
    }
    const ClassTable& get_class_table() const {
        return cls_table;
    }

    FunctionPool& get_function_pool() {
        return fun_pool;
    }
    const FunctionPool& get_function_pool() const {
        return fun_pool;
    }

    Engine* get_engine() { return engine; }

private:
    HSQUIRRELVM vm = nullptr;

    HSQOBJECT root_;

    ClassTable cls_table;
    FunctionPool fun_pool;

    // Native modules
    SQObject math_module;
    SQObject string_module;
    SQObject blob_module;

    std::vector<std::string> running_scripts;

    StableResourcePool<ScriptModule>* script_res;
    Engine* engine;

    phmap::flat_hash_map<std::string, Ref<ScriptModule>> modules;

    template<bool must_exist> static SQInteger sq_require(HSQUIRRELVM vm);

    enum class CompileScriptResult {
        Ok,
        FileNotFound,
        CompilationFailed
    };

    CompileScriptResult compile_script(const char* resolved_fn, const char* orig_fn, const HSQOBJECT* bindings,
                                       HSQOBJECT* script_closure, std::string& err);

    void register_native_module(SQFUNCTION reg_func, HSQOBJECT& mod);
};

#endif //THESYSTEM_VM_H
