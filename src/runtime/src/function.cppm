export module l3.runtime:function;

import utils;

import :identifier;
import :ref_value;
import std;

export namespace l3::runtime {

class Value;

using L3Args = std::span<const Ref>;

class BuiltinFunction {
public:
  using Body = std::function<Ref(L3Args)>;

private:
  Identifier name;
  Body body;

public:
  BuiltinFunction(Identifier &&name, Body body)
      : name{std::move(name)}, body{std::move(body)} {}

  [[nodiscard]] Ref invoke(L3Args args) const { return body(args); }

  DEFINE_ACCESSOR_X(name);
};

struct BytecodeFunctionId {
  std::size_t id;
  std::string name;
  std::size_t arity;
  std::vector<Ref> upvalues;
  std::vector<Ref> curried_args;
};

class Function {
  std::variant<BuiltinFunction, BytecodeFunctionId> inner;

public:
  Function(BuiltinFunction &&function);
  Function(BytecodeFunctionId &&function);

  [[nodiscard]] utils::optional_cref<BuiltinFunction>
  as_builtin_function() const {
    if (const auto *builtin_function = std::get_if<BuiltinFunction>(&inner)) {
      return *builtin_function;
    }
    return std::nullopt;
  }

  [[nodiscard]] utils::optional_ref<BytecodeFunctionId>
  as_mut_bytecode_function() {
    if (auto *bytecode_function = std::get_if<BytecodeFunctionId>(&inner)) {
      return *bytecode_function;
    }
    return std::nullopt;
  }

  [[nodiscard]] utils::optional_cref<BytecodeFunctionId>
  as_bytecode_function() const {
    if (const auto *bytecode_function =
            std::get_if<BytecodeFunctionId>(&inner)) {
      return *bytecode_function;
    }
    return std::nullopt;
  }

  VISIT(inner);
};

} // namespace l3::runtime
