//
// Created by lasagnaphil on 9/18/2021.
//

#include "vm.h"

#include <physfs.h>

#include "engine.h"
#include "core/log.h"
#include "core/file.h"
#include "core/defer.h"
#include "core/strutil.h"
#include "squirrel/utils.h"

#include <cstdarg>

static void squirrel_compile_error_handler(
        HSQUIRRELVM vm, const SQChar* desc, const SQChar* source, SQInteger line, SQInteger column) {

    std::string real_source = fmt::format("assets/{}", source);
    g_logger.log(LOG_ERROR, real_source.c_str(), line, "Compile error ({},{}): {}", line, column, desc);
}

static SQInteger squirrel_runtime_error_handler(HSQUIRRELVM vm) {
    SQStackInfos si;
    sq_stackinfos(vm, 1, &si);

    const SQChar* source = si.source != nullptr ? si.source : "null";
    const SQChar* funcname = si.funcname != nullptr ? si.funcname : "unknown";
    std::string real_source = fmt::format("assets/{}", source);

    const SQChar *err_str = "unknown error";
    if (sq_gettop(vm) >= 1) {
        sq_getstring(vm, 2, &err_str);
    }

    g_logger.log(LOG_ERROR, real_source.c_str(), si.line, "Runtime error in {}: {}", funcname, err_str);
    return 0;
}

static void squirrel_print_handler(HSQUIRRELVM vm, const SQChar* s, ...) {
    va_list args;
    va_start(args, s);
    vfprintf(stdout, s, args);
    fputc('\n', stdout);
    fflush(stdout);
    // log_log(LOG_INFO, __FILE__, __LINE__, s, args);
    va_end(args);
}

static void squirrel_error_handler(HSQUIRRELVM vm, const SQChar* s, ...) {
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    fputc('\n', stderr);
    fflush(stderr);
    // log_log(LOG_INFO, __FILE__, __LINE__, s, args);
    va_end(args);
}

static SQInteger keepref(HSQUIRRELVM vm)
{
    // arguments: this, object, ref holder
    sq_push(vm, 2); // push object
    sq_arrayappend(vm, 3); // append to ref holder
    sq_push(vm, 2); // push object again
    return 1;
}

void VM::init(Engine* _engine) {
    this->script_res = &_engine->get_resources()->get_pool<ScriptModule>();
    this->engine = _engine;

    // Initialize Squirrel
    vm = sq_open(1024);
    sq_setcompilererrorhandler(vm, squirrel_compile_error_handler);
    sq_setprintfunc(vm, squirrel_print_handler, squirrel_error_handler);
    sq_newclosure(vm, squirrel_runtime_error_handler, 0);
    sq_seterrorhandler(vm);

    // Register base libs
    register_native_module(sqstd_register_mathlib, math_module);
    register_native_module(sqstd_register_stringlib, string_module);
    register_native_module(sqstd_register_bloblib, blob_module);

    // Set root table
    sq_pushroottable(vm);
    sq_getstackobj(vm, -1, &root_);
    sq_pop(vm, 1);

    // Set foreign pointer to engine
    sq_setsharedforeignptr(vm, engine);
}

void VM::release() {
    sq_close(vm);
    vm = nullptr;
    script_res->clear();
}

VM::~VM() {
    if (vm != nullptr) {
        release();
    }
}

template<bool must_exist>
SQInteger VM::sq_require(HSQUIRRELVM vm) {
    SQUserPointer self_ptr = nullptr;
    if (SQ_FAILED(sq_getuserpointer(vm, -1, &self_ptr)) || !self_ptr)
        return sq_throwerror(vm, "No module manager");

    VM *self = reinterpret_cast<VM*>(self_ptr);
    assert(self->vm == vm);

    const char *filename = nullptr;
    SQInteger filename_len = -1;
    sq_getstringandsize(vm, 2, &filename, &filename_len);

    if (strcmp(filename, "math") == 0) {
        sq_pushobject(vm, self->math_module);
        return 1;
    }
    if (strcmp(filename, "string") == 0) {
        sq_pushobject(vm, self->string_module);
        return 1;
    }
    if (strcmp(filename, "blob") == 0) {
        sq_pushobject(vm, self->blob_module);
        return 1;
    }

    std::string errMsg;
    auto script_id = self->require_module(filename, must_exist, nullptr);
    if (!script_id) {
        std::string err = "Script ";
        err += filename;
        err += " load failed!";
        return sq_throwerror(vm, err.c_str());
    }
    auto script = self->script_res->get(script_id);

    sq_pushobject(vm, script->exports);
    return 1;
}

// TODO: Add unit tests for this
std::string remove_relative_paths(std::string path) {
    size_t j = 0;
    while (true) {
        j = path.find("../", j);
        if (j == 0) {
            path.erase(j, 3);
        }
        if (j == std::string::npos) {
            break;
        }
        int i = path.rfind('/', j-2);
        path.erase(i+1, j+2-i);
        j = i+1;
    }
    return path;
}

std::string VM::resolve_file_name(const char* modpath) {
    std::string res;

    std::string script_folder = "scripts/";

    // Try relative path
    if (!running_scripts.empty()) {
        auto current_script = running_scripts.back();
        int i = current_script.find_last_of('/');
        if (i != std::string::npos) {
            std::string current_folder = current_script.substr(0, i) + '/';
            auto modpath_rel_full = current_folder + modpath + ".nut";
            auto modpath_full = remove_relative_paths(modpath_rel_full);
            if (PHYSFS_exists(modpath_full.c_str())) {
                return modpath_full;
            }
        }
    }

    // Try absolute path
    // If used full path from assets folder...
    std::string modpath_abs_full = modpath;
    if (!strutil::starts_with(modpath, script_folder)) {
        modpath_abs_full = script_folder + modpath_abs_full;
    }
    if (!strutil::ends_with(modpath, ".nut")) {
        modpath_abs_full = modpath_abs_full + ".nut";
    }
    if (PHYSFS_exists(modpath_abs_full.c_str())) {
        return modpath_abs_full;
    }
    return ""; // cannot find file
}

bool VM::check_circular_refs(const char* filename) {
    for (auto& script : running_scripts) {
        if (script == filename) return false;
    }
    return true;
}

Ref<ScriptModule> VM::require_module(const char* requested_fn, bool must_exist, const char* __name__) {
    auto resolved_fn = resolve_file_name(requested_fn);
    if (resolved_fn.empty()) {
        log_error("Cannot resolve imported module name {}", requested_fn);
        return {};
    }
    if (!check_circular_refs(resolved_fn.c_str())) {
        log_error("Circular references error in requested script {}", requested_fn);
        return {};
    }

    auto it = modules.find(resolved_fn);
    if (it != modules.end()) {
        return it->second;
    }

    SQInteger prev_top = sq_gettop(vm);

    sq_newtable(vm);
    HSQOBJECT bindings;
    sq_getstackobj(vm, -1, &bindings);
    sq_addref(vm, &bindings);

    assert(sq_gettop(vm) == prev_top + 1);

    sq_newarray(vm, 0);
    HSQOBJECT ref_holder;
    sq_getstackobj(vm, -1, &ref_holder);
    sq_addref(vm, &ref_holder);
    sq_poptop(vm);

    sq_pushstring(vm, "keepref", 7);
    sq_pushobject(vm, ref_holder);
    sq_newclosure(vm, keepref, 1);
    sq_rawset(vm, -3);

    sq_pushstring(vm, "__name__", 8);
    sq_pushstring(vm, __name__, -1);
    sq_rawset(vm, -3);

    assert(sq_gettop(vm) == prev_top + 1); //bindings table

    // bind require api
    sq_pushobject(vm, bindings);

    sq_pushstring(vm, _SC("require"), 7);
    sq_pushuserpointer(vm, this);
    sq_newclosure(vm, sq_require<true>, 1);
    sq_setparamscheck(vm, 2, _SC(".s"));
    sq_rawset(vm, -3);

    sq_pushstring(vm, _SC("require_optional"), 16);
    sq_pushuserpointer(vm, this);
    sq_newclosure(vm, sq_require<false>, 1);
    sq_setparamscheck(vm, 2, _SC(".s"));
    sq_rawset(vm, -3);

    sq_poptop(vm);

    assert(sq_gettop(vm) == prev_top + 1); //bindings table
    sq_poptop(vm);

    HSQOBJECT script_closure, exports;
    std::string err;
    VM::CompileScriptResult compile_result = compile_script(resolved_fn.c_str(), requested_fn, &bindings, &script_closure, err);
    if (!must_exist && compile_result == CompileScriptResult::FileNotFound) {
        log_error("Script {} not found!", resolved_fn);
        return {};
    }
    if (compile_result == CompileScriptResult::CompilationFailed) {
        log_error("Compliation failed for script {}!", resolved_fn);
        return {};
    }

    if (__name__ == nullptr) {
        __name__ = resolved_fn.c_str();
    }

    size_t rs_idx = running_scripts.size();
    running_scripts.push_back(resolved_fn.c_str());

    sq_pushobject(vm, script_closure);

    sq_newtable(vm);
    assert(sq_gettype(vm, -1) == OT_TABLE);
    HSQOBJECT obj_this;
    sq_getstackobj(vm, -1, &obj_this);
    sq_addref(vm, &obj_this);

    SQRESULT call_res = sq_call(vm, 1, true, true);

    assert(running_scripts.size() == rs_idx+1);
    running_scripts.pop_back();

    if (SQ_FAILED(call_res)) {
        log_error("Failed to run script {}", resolved_fn.c_str());
        sq_pop(vm, 1); // clojure, no return value on error
        return {};
    }

    sq_getstackobj(vm, -1, &exports);
    sq_addref(vm, &exports);
    sq_pop(vm, 2); // retval + closure

    assert(sq_gettop(vm) == prev_top);

    bool cached;
    auto mod = script_res->new_item();
    auto mod_ptr = mod.get();
    mod_ptr->exports = exports;
    mod_ptr->fn = resolved_fn;
    // module.state_storage = stage_storage;
    mod_ptr->module_this = obj_this;
    mod_ptr->ref_holder = ref_holder;
    mod_ptr->__name__ = __name__;

    return mod;
}

VM::CompileScriptResult
VM::compile_script(const char* resolved_fn, const char* requested_fn, const HSQOBJECT* bindings, HSQOBJECT* script_closure,
                   std::string& err) {
    auto f = PHYSFS_openRead(resolved_fn);
    if (!f) {
        err = std::string("Script file not found: ") + requested_fn + " / " + resolved_fn;
        return CompileScriptResult::FileNotFound;
    }

    PHYSFS_sint64 file_size = PHYSFS_fileLength(f);
    std::vector<char> buf(file_size+1);
    PHYSFS_read(f, buf.data(), 1, file_size);
    buf[file_size] = 0;
    PHYSFS_close(f);

    if (SQ_FAILED(sq_compilebuffer(vm, buf.data(), file_size, resolved_fn, true, bindings))) {
        err = std::string("Failed to compile file: ") + requested_fn +" / " + resolved_fn;
        return CompileScriptResult::CompilationFailed;
    }

    sq_getstackobj(vm, -1, script_closure);
    sq_addref(vm, script_closure);
    sq_pop(vm, 1);

    return VM::CompileScriptResult::Ok;
}

void VM::register_native_module(SQFUNCTION reg_func, HSQOBJECT& mod) {
    sq_newtable(vm);
    sq_getstackobj(vm, -1, &mod);
    sq_addref(vm, &mod);
    reg_func(vm);
    sq_pop(vm, 1);
}

/*
Module* search_module(const std::vector<std::string>& modpath, bool absolute) {
    Module* cur_running_mod;

    // Try relative paths first
    if (!running_scripts.empty()) {
        cur_running_mod = running_scripts.back();
    }
    else {
        cur_running_mod = root_module.get();
    }

    Module* search_mod = cur_running_mod;
    bool found = true;
    for (auto& token : modpath) {
        auto it = cur_running_mod->submodules.find(token);
        if (it != cur_running_mod->submodules.end()) {
            search_mod = &it->second;
        }
        else {
            found = false;
            break;
        }
    }
    if (found) {
        return search_mod;
    }
    else {
        return nullptr;
    }
}
 */
