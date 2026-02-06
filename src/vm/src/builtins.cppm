module;

#include <initializer_list>

export module l3.vm:builtins;

import :function;

export namespace l3::vm::builtins {

extern const std::initializer_list<
    std::pair<std::string_view, BuiltinFunction::Body>>
    BUILTINS;

}
