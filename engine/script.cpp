//
// Created by lasagnaphil on 2021-07-31.
//

#include "script.h"

#include "squirrel.h"

#include "engine.h"
#include "core/log.h"
#include "core/file.h"

/*
Id<Script> Script::load(HSQUIRRELVM vm, Resources& res, const char* filename) {
    Script script;
    script.filename = filename;
    SQRESULT result = script.reload(vm);
    if (SQ_FAILED(result)) {
        return Id<Script>::null();
    }
    return res.scripts.insert(script);
}

SQRESULT Script::reload(HSQUIRRELVM vm) {
    std::vector<char> buf = load_file_to_buffer(this->filename.c_str());
    if (buf.empty()) return SQ_ERROR;
    SQRESULT result = sq_compilebuffer(vm, buf.data(), buf.size(), this->filename.c_str(), true);
    if (SQ_FAILED(result)) return result;

    sq_getstackobj(vm, -1, &this->object);
    sq_addref(vm, &this->object);
    sq_pop(vm, 1);

    return result;
}

void Script::run(HSQUIRRELVM vm) {
    if (sq_isnull(this->object)) {
        log_error("Trying to run an empty script %s", this->filename.c_str());
        return;
    }

    SQInteger top = sq_gettop(vm);
    sq_pushobject(vm, this->object);
    sq_pushroottable(vm);
    SQRESULT result = sq_call(vm, 1, false, true);
    sq_settop(vm, top);
    if (SQ_FAILED(result)) {
        log_error("Failed to execute script %s", this->filename.c_str());
    }
}
 */


