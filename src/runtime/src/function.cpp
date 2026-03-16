module l3.runtime;

namespace l3::runtime {

Function::Function(BuiltinFunction &&function) : inner{std::move(function)} {}
Function::Function(BytecodeFunctionId &&function)
    : inner{std::move(function)} {}

} // namespace l3::runtime
