#pragma once

#include "scriptable.h"
#include "squirrel/utils.h"
#include "squirrel/vm.h"

template<class T>
void Scriptable<T>::register_script_methods(SQInteger instance_idx) {
    instance.reset();
    on_update.reset();
    on_render.reset();

    auto vm = Engine::instance().get_vm()->handle();

    sq_getstackobj(vm, instance_idx, &instance.obj);

    auto top = sq_gettop(vm);

    sq_getclass(vm, instance_idx);

    // sq::Class cls;
    // sq_getstackobj(vm, -1, &cls.obj);
    sq_pushstring(vm, "update", -1);
    if (SQ_SUCCEEDED(sq_get(vm, top+1))) {
        sq_getstackobj(vm, -1, &on_update.obj);
    }
    sq_pushstring(vm, "render", -1);
    if (SQ_SUCCEEDED(sq_get(vm, top+1))) {
        sq_getstackobj(vm, -1, &on_render.obj);
    }

    sq_settop(vm, top);
}

template <class T>
struct has_update_method
{
    template <class U, size_t (U::*)() const> struct SFINAE {};
    template <class U> static char Test(SFINAE<U, &U::update>*);
    template <class U> static int Test(...);
    static const bool value = sizeof(Test<T>(0)) == sizeof(char);
};
template <class T>
struct has_render_method
{
    template <class U, size_t (U::*)() const> struct SFINAE {};
    template <class U> static char Test(SFINAE<U, &U::render>*);
    template <class U> static int Test(...);
    static const bool value = sizeof(Test<T>(0)) == sizeof(char);
};

template<class T>
void Scriptable<T>::update(VM &vm, float dt) {
    /*
    if constexpr (has_update_method<T>::value) {
        T::update(dt);
    }
     */
    if (!on_update.is_null()) {
        vm.call_func(on_update, instance, dt);
    }
}

template<class T>
void Scriptable<T>::render(VM &vm) {
    /*
    if constexpr (has_render_method<T>::value) {
        T::render();
    }
     */
    if (!on_render.is_null()) {
        vm.call_func(on_render, instance);
    }
}
