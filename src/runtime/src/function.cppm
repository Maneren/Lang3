export module l3.runtime:function;

import std;

import utils;

import l3.ast;

import :stack_value;

export namespace l3::runtime {

class UpvalueCell;

using L3Args = std::span<const StackValue>;

class BuiltinFunction {
public:
  using Body = std::function<StackValue(L3Args)>;

private:
  ast::Identifier name;
  Body body;

public:
  BuiltinFunction(ast::Identifier &&name, Body body)
      : name{std::move(name)}, body{std::move(body)} {}

  [[nodiscard]] StackValue invoke(L3Args args) const;

  DEFINE_ACCESSOR_X(name);
};

struct BytecodeFunction {
  std::size_t id;
  std::string name;
  std::size_t arity;
  std::vector<StackValue> curried_args;
  std::vector<UpvalueCell *> captured_upvalue_refs;
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
