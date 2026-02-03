module;

#include <memory>
#include <optional>
#include <span>
#include <utils/accessor.h>
#include <utils/visit.h>
#include <variant>

export module l3.vm:function;

import l3.ast;
import utils;
import :identifier;
import :ref_value;
import :scope;

export namespace l3::vm {

class VM;
class Value;

using L3Args = std::span<const RefValue>;

class L3Function {
  std::shared_ptr<ScopeStack> captures;
  std::unique_ptr<Scope> curried_arguments;
  std::reference_wrapper<const ast::FunctionBody> body;
  std::optional<Identifier> name;

public:
  L3Function(const L3Function &) = delete;
  L3Function(L3Function &&) = default;
  L3Function &operator=(const L3Function &) = delete;
  L3Function &operator=(L3Function &&) = default;
  L3Function(
      std::shared_ptr<ScopeStack> captures, const ast::NamedFunction &function
  );
  L3Function(
      std::shared_ptr<ScopeStack> captures,
      const ast::AnonymousFunction &function
  );
  L3Function(
      std::shared_ptr<ScopeStack> captures,
      Scope &&curried_arguments,
      std::reference_wrapper<const ast::FunctionBody> body,
      std::optional<Identifier> name
  );
  ~L3Function();

  RefValue operator()(VM &vm, L3Args args);

  DEFINE_VALUE_ACCESSOR_X(body);
  DEFINE_ACCESSOR_X(captures);
  [[nodiscard]] const Identifier &get_name() const;
  [[nodiscard]] const std::unique_ptr<Scope> &get_curried_arguments() const {
    return curried_arguments;
  }

private:
  static Identifier anonymous_function_name;
};

class BuiltinFunction {
public:
  using Body = RefValue (*)(VM &vm, L3Args args);

private:
  Identifier name;
  Body body;

public:
  BuiltinFunction(Identifier &&name, Body body);

  RefValue operator()(VM &vm, L3Args args) const;

  DEFINE_ACCESSOR_X(name);
};

class Function {
public:
  Function(L3Function &&function);
  Function(BuiltinFunction &&function);

  RefValue operator()(VM &vm, L3Args args);

  VISIT(inner);

private:
  std::variant<L3Function, BuiltinFunction> inner;
};

} // namespace l3::vm
