module l3.runtime;

import :value;

namespace l3::runtime {

StackValue BuiltinFunction::invoke(L3Args args) const { return body(args); }

Function::Function(BuiltinFunction &&function) : inner{std::move(function)} {}
Function::Function(BytecodeFunction &&function) : inner{std::move(function)} {}

utils::optional_cref<BuiltinFunction> Function::as_builtin_function() const {
  if (const auto *builtin_function = std::get_if<BuiltinFunction>(&inner)) {
    return *builtin_function;
  }
  return std::nullopt;
}

utils::optional_ref<BytecodeFunction> Function::as_mut_bytecode_function() {
  if (auto *bytecode_function = std::get_if<BytecodeFunction>(&inner)) {
    return *bytecode_function;
  }
  return std::nullopt;
}

utils::optional_cref<BytecodeFunction> Function::as_bytecode_function() const {
  if (const auto *bytecode_function = std::get_if<BytecodeFunction>(&inner)) {
    return *bytecode_function;
  }
  return std::nullopt;
}

} // namespace l3::runtime
