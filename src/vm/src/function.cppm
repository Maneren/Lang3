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

export namespace l3::vm {

class VM;
class Value;
class Scope;
class ScopeStack;

using L3Args = std::span<const RefValue>;

class L3Function {
  std::shared_ptr<ScopeStack> captures;
  std::shared_ptr<Scope> curried_arguments;
  std::shared_ptr<ast::FunctionBody> body;
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
      std::shared_ptr<ast::FunctionBody> body,
      std::optional<Identifier> name
  );
  ~L3Function();

  RefValue operator()(VM &vm, L3Args args);

  DEFINE_ACCESSOR_X(body);
  DEFINE_ACCESSOR_X(captures);
  DEFINE_PTR_ACCESSOR_X(curried_arguments);
  [[nodiscard]] const Identifier &get_name() const;

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
