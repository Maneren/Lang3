export module l3.vm:builtins;

import l3.runtime;
import std;

export namespace l3::vm {
class BytecodeVM;
}

export namespace l3::builtins {

extern const std::initializer_list<std::pair<
    std::string_view,
    runtime::Ref (*)(l3::vm::BytecodeVM &, runtime::L3Args)>>
    BUILTINS;

} // namespace l3::builtins
