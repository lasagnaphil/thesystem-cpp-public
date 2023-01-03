//
// Created by lasagnaphil on 9/18/2021.
//

#ifndef THESYSTEM_UTILS_H
#define THESYSTEM_UTILS_H

#include <string>

#include "core/log.h"
#include "squirrel.h"

#define sq_print_stack(vm) { sq_print_stack_internal(vm, __FILE__, __LINE__); }
void sq_print_stack_internal(HSQUIRRELVM vm, const char* file, int line);

std::string sqx_to_string_basic(HSQOBJECT o);
std::string sqx_to_string(HSQUIRRELVM vm, HSQOBJECT o, bool multiline = true, int indent = 4);

SQRESULT sqx_register_fns(HSQUIRRELVM vm, SQRegFunction* fns);

SQRESULT sqx_register_int(HSQUIRRELVM vm, SQInteger idx, const SQChar* s, int i);

SQRESULT sqx_register_float(HSQUIRRELVM vm, SQInteger idx, const SQChar* s, float f);

SQRESULT sqx_register_string(HSQUIRRELVM vm, SQInteger idx, const SQChar* s, const char* str);


/*
SClass make_class(HSQUIRRELVM vm, const char* name);

void release_class(HSQUIRRELVM vm, SClass& klass);

void close_class(HSQUIRRELVM vm, SClass& klass);
 */

#endif //THESYSTEM_UTILS_H
