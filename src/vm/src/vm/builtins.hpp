#pragma once

#include "vm/function.hpp"
#include <initializer_list>
#include <string_view>
#include <utility>

namespace l3::vm::builtins {

extern const std::initializer_list<
    std::pair<std::string_view, BuiltinFunction::Body>>
    BUILTINS;

}
