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
  std::optional<Scope> curried;
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
      Scope &&curried,
      std::reference_wrapper<const ast::FunctionBody> body,
      std::optional<Identifier> name
  );
  ~L3Function();

  L3Function curry(Scope &&arguments) const {
    return L3Function{captures, std::move(arguments), body, name};
  }

  DEFINE_VALUE_ACCESSOR_X(body);
  DEFINE_ACCESSOR_X(captures);
  [[nodiscard]] const Identifier &get_name() const {
    if (name.has_value()) {
      return name.value();
    }
    return anonymous_function_name;
  }
  [[nodiscard]] const utils::optional_cref<Scope> get_curried() const {
    return curried;
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

  RefValue invoke(VM &vm, L3Args args) const { return body(vm, args); }

  DEFINE_ACCESSOR_X(name);
  DEFINE_ACCESSOR_X(body);
};

class Function {
  std::variant<L3Function, BuiltinFunction> inner;

public:
  Function(L3Function &&function);
  Function(BuiltinFunction &&function);

  VISIT(inner);
};

} // namespace l3::vm
