#pragma once

#include <squirrel.h>
#include "core/reflect.h"
#include <string>

CLASS(Resource) ScriptModule {
public:
    std::string fn;
    std::string __name__;
    SQObject exports, state_storage, ref_holder, module_this;
};
