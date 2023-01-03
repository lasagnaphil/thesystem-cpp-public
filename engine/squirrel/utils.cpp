//
// Created by lasagnaphil on 9/18/2021.
//

#include "utils.h"

void sq_print_stack_internal(HSQUIRRELVM vm, const char* file, int line) {
    std::string msg = "stack> \n";
    int top = sq_gettop(vm);
    for (int i = 1; i <= top; i++) {
        HSQOBJECT o;
        sq_getstackobj(vm, i, &o);
        std::string value = sqx_to_string_basic(o);
        msg += value;
        if (i < top) {
            msg += '\n';
        }
    }
    g_logger.log_raw(LOG_DEBUG, file, line, msg);
}

std::string sqx_to_string_basic(HSQOBJECT o) {
    switch(sq_type(o)) {
        case OT_NULL: return "null";
        case OT_INTEGER: return std::to_string(sq_objtointeger(&o));
        case OT_FLOAT: return std::to_string(sq_objtofloat(&o));
        case OT_BOOL: return sq_objtobool(&o)? "true" : "false";
        case OT_STRING: return fmt::format("\"{}\"", sq_objtostring(&o));
        case OT_TABLE: return "table";
        case OT_ARRAY: return "array";
        case OT_USERDATA: return "userdata";
        case OT_CLOSURE: return "closure";
        case OT_NATIVECLOSURE: return "nativeclosure";
        case OT_GENERATOR: return "generator";
        case OT_USERPOINTER: return "userpointer";
        case OT_THREAD: return "thread";
        case OT_FUNCPROTO: return "funcproto";
        case OT_CLASS: return "class";
        case OT_INSTANCE: return "instance";
        case OT_WEAKREF: return "weakref";
        case OT_OUTER: return "outer";
    }
}

std::string to_string_internal(HSQUIRRELVM vm, HSQOBJECT o, bool multiline, int indent, int cur_indent = 0, bool parens = true) {
    switch(sq_type(o)) {
        case OT_NULL: return "null";
        case OT_INTEGER: return std::to_string(sq_objtointeger(&o));
        case OT_FLOAT: return std::to_string(sq_objtofloat(&o));
        case OT_BOOL: return sq_objtobool(&o)? "true" : "false";
        case OT_STRING: {
            auto s = sq_objtostring(&o);
            return parens? fmt::format("\"{}\"", s) : s;
        }
        case OT_TABLE: {
            std::string s = "{";
            if (multiline) s += '\n';
            SQInteger top = sq_gettop(vm);
            sq_pushobject(vm, o);
            sq_pushnull(vm);
            while (SQ_SUCCEEDED(sq_next(vm, top+1))) {
                HSQOBJECT key, value;
                sq_getstackobj(vm, -2, &key);
                sq_getstackobj(vm, -1, &value);
                for (int i = 0; i < cur_indent + indent; i++) s += ' ';
                s += fmt::format("{} = {}, ",
                                 to_string_internal(vm, key, multiline, indent, cur_indent + indent, false),
                                 to_string_internal(vm, value, multiline, indent, cur_indent + indent));
                if (multiline) s += '\n';
                sq_pop(vm, 2);
            }
            for (int i = 0; i < cur_indent; i++) s += ' ';
            s += '}';
            sq_settop(vm, top);
            return s;
        }
        case OT_ARRAY: {
            std::string s = "[";
            if (multiline) s += '\n';
            SQInteger top = sq_gettop(vm);
            sq_pushobject(vm, o);
            sq_pushnull(vm);
            while (SQ_SUCCEEDED(sq_next(vm, top+1))) {
                HSQOBJECT value;
                sq_getstackobj(vm, -1, &value);
                if (multiline) for (int i = 0; i < cur_indent + indent; i++) s += ' ';
                s += to_string_internal(vm, value, multiline, indent, cur_indent + indent) + ",";
                if (multiline) s += '\n';
                sq_pop(vm, 2);
            }
            if (multiline) for (int i = 0; i < indent; i++) s += ' ';
            s += ']';
            sq_settop(vm, top);
            return s;
        }
        case OT_USERDATA: return "<userdata>";
        case OT_CLOSURE: return "<closure>";
        case OT_NATIVECLOSURE: return "<nativeclosure>";
        case OT_GENERATOR: return "<generator>";
        case OT_USERPOINTER: return "<userpointer>";
        case OT_THREAD: return "<thread>";
        case OT_FUNCPROTO: return "<funcproto>";
        case OT_CLASS: return "<class>";
        case OT_INSTANCE: return "<instance>";
        case OT_WEAKREF: return "<weakref>";
        case OT_OUTER: return "<outer>";
    }
}

std::string sqx_to_string(HSQUIRRELVM vm, HSQOBJECT o, bool multiline, int indent) {
    return to_string_internal(vm, o, multiline, indent, 0);
}

SQRESULT sqx_register_fns(HSQUIRRELVM vm, SQRegFunction* fns) {
    int i = 0;
    while (fns[i].name != nullptr) {
        sq_pushstring(vm, fns[i].name, -1);
        sq_newclosure(vm, fns[i].f, SQFalse);
        sq_setparamscheck(vm, fns[i].nparamscheck, fns[i].typemask);
        sq_setnativeclosurename(vm, -1, fns[i].name);
        sq_newslot(vm, -3, SQFalse);
        i++;
    }
    return SQ_OK;
}

SQRESULT sqx_register_int(HSQUIRRELVM vm, SQInteger idx, const SQChar* s, int i) {
    sq_pushstring(vm, s,-1);
    sq_pushinteger(vm, i);
    sq_newslot(vm, -3, SQFalse);
    return SQ_OK;
}

SQRESULT sqx_register_float(HSQUIRRELVM vm, SQInteger idx, const SQChar* s, float f) {
    sq_pushstring(vm, s,-1);
    sq_pushfloat(vm, f);
    sq_newslot(vm, -3, SQFalse);
    return SQ_OK;
}

SQRESULT sqx_register_string(HSQUIRRELVM vm, SQInteger idx, const SQChar* s, const char* str) {
    sq_pushstring(vm, s, -1);
    sq_pushstring(vm, str, strlen(str));
    sq_newslot(vm, -3, SQFalse);
    return SQ_OK;
}

HSQOBJECT make_accessor_table(HSQUIRRELVM vm) {
    HSQOBJECT tableobj;
    sq_newtable(vm);
    sq_getstackobj(vm, -1, &tableobj);
    sq_addref(vm, &tableobj);
    sq_pop(vm, 1);
    return tableobj;
}

void defmetamethod(HSQUIRRELVM vm, const SQChar* mm, HSQOBJECT table, SQFUNCTION f) {
    sq_pushstring(vm, mm, -1);
    sq_pushobject(vm, table);
    sq_newclosure(vm, f, 1);
    sq_newslot(vm, -3, false);
}

SQInteger delegate_get(HSQUIRRELVM vm) {
    sq_push(vm, 2);
    if (!SQ_SUCCEEDED(sq_get(vm, -2))) {
        const SQChar* s;
        sq_getstring(vm, 2, &s);
        auto msg = fmt::format("member variable {} not found", s);
        return sq_throwerror(vm, msg.c_str());
    }
    sq_push(vm, 1);

    sq_call(vm, 1, SQTrue, SQTrue);
    return 1;
}

SQInteger delegate_set(HSQUIRRELVM vm) {
    sq_push(vm, 2);
    if (!SQ_SUCCEEDED(sq_get(vm, -2))) {
        const SQChar* s;
        sq_getstring(vm, 2, &s);
        auto msg = fmt::format("member variable {} not found", s);
        return sq_throwerror(vm, msg.c_str());
    }

    sq_push(vm, 1);
    sq_push(vm, 3);

    sq_call(vm, 2, SQTrue, SQTrue);
    return 0;
}

/*
SClass make_class(HSQUIRRELVM vm, const char* name) {
    SClass klass;
    klass.name = name;
    klass.closed = false;
    sq_newclass(vm, SQFalse);
    sq_getstackobj(vm, -1, &klass.sqclass);
    sq_addref(vm, &klass.sqclass);
    klass.getter_table = make_accessor_table(vm);
    klass.setter_table = make_accessor_table(vm);
    defmetamethod(vm, _SC("_get"), klass.getter_table, delegate_get);
    defmetamethod(vm, _SC("_set"), klass.setter_table, delegate_set);
    return klass;
}

void release_class(HSQUIRRELVM vm, SClass& klass) {
    sq_release(vm, &klass.sqclass);
    sq_release(vm, &klass.getter_table);
    sq_release(vm, &klass.setter_table);
}

void close_class(HSQUIRRELVM vm, SClass& klass) {
    if (klass.closed) return;

    sq_pushroottable(vm);
    sq_pushstring(vm, klass.name.c_str(), -1);
    sq_pushobject(vm, klass.sqclass);
    sq_newslot(vm, -3, SQFalse);
    klass.closed = true;
}
 */
