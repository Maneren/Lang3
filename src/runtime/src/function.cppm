export module l3.runtime:function;

import utils;
import :identifier;
import :stack_value;
import std;

export namespace l3::runtime {

class GCUpvalue;

using L3Args = std::span<const StackValue>;

class BuiltinFunction {
public:
  using Body = std::function<Value(L3Args)>;

private:
  Identifier name;
  Body body;

public:
  BuiltinFunction(Identifier &&name, Body body)
      : name{std::move(name)}, body{std::move(body)} {}

  [[nodiscard]] Value invoke(L3Args args) const;

  DEFINE_ACCESSOR_X(name);
};

struct BytecodeFunction {
  std::size_t id;
  std::string name;
  std::size_t arity;
  std::vector<StackValue> curried_args;
  std::vector<GCUpvalue *> captured_upvalue_refs;
};

class Function {
  std::variant<BuiltinFunction, BytecodeFunction> inner;

public:
  Function(BuiltinFunction &&function);
  Function(BytecodeFunction &&function);

  [[nodiscard]] utils::optional_cref<BuiltinFunction>
  as_builtin_function() const;

  [[nodiscard]] utils::optional_ref<BytecodeFunction>
  as_mut_bytecode_function();

  [[nodiscard]] utils::optional_cref<BytecodeFunction>
  as_bytecode_function() const;

  VISIT(inner);
};

} // namespace l3::runtime
