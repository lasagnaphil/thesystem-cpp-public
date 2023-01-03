#pragma once

#include "squirrel/object.h"

#include <type_traits>

class VM;

template <class T>
class Scriptable : public T {
private:
    sq::Instance instance;

    sq::Function on_update;
    sq::Function on_render;

public:
    using wrapped_type = T;

    Scriptable() = default;
    Scriptable(const typename T::Options& opt) : T(opt) {}

    void register_script_methods(SQInteger instance_idx);

    void update(VM& vm, float dt);

    void render(VM& vm);

};

template <class T>
struct is_scriptable {
    static constexpr bool value = false;
};

template <template <class...> class C, class T>
struct is_scriptable<C<T>> {
    static constexpr bool value = std::is_same<C<T>, Scriptable<T>>::value;
};

template <class T>
constexpr bool is_scriptable_v = is_scriptable<T>::value;
