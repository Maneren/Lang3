module l3.runtime;

import :value;

namespace l3::runtime {

Value BuiltinFunction::invoke(L3Args args) const { return body(args); }

Function::Function(BuiltinFunction &&function) : inner{std::move(function)} {}
Function::Function(BytecodeFunction &&function) : inner{std::move(function)} {}

} // namespace l3::runtime
