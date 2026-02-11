export module l3.vm:builtins;

import :function;
import std;

export namespace l3::vm::builtins {

extern const std::initializer_list<
    std::pair<std::string_view, BuiltinFunction::Body>>
    BUILTINS;

}
