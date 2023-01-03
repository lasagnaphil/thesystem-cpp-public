//
// Created by lasagnaphil on 10/11/2021.
//

#include "squirrel/args.h"
#include "squirrel/vm.h"
#include "engine.h"

ClassTable& sq::get_class_table(HSQUIRRELVM vm) {
    Engine* engine = reinterpret_cast<Engine*>(sq_getsharedforeignptr(vm));
    return engine->get_vm()->get_class_table();
}

FunctionPool& sq::get_function_pool(HSQUIRRELVM vm) {
    Engine* engine = reinterpret_cast<Engine*>(sq_getsharedforeignptr(vm));
    return engine->get_vm()->get_function_pool();
}
