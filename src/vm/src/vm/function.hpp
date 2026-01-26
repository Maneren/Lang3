#pragma once

#include "utils/accessor.h"
#include "vm/value.hpp"
#include <ast/nodes/function.hpp>
#include <memory>
#include <optional>
#include <vector>

namespace l3::vm {

class Scope;
class VM;

using L3Args = std::span<RefValue>;

class L3Function {
public:
  L3Function(const L3Function &) = delete;
  L3Function(L3Function &&) = default;
  L3Function &operator=(const L3Function &) = delete;
  L3Function &operator=(L3Function &&) = default;
  L3Function(
      const std::vector<std::shared_ptr<Scope>> &active_scopes,
      const ast::NamedFunction &function
  );
  L3Function(
      const std::vector<std::shared_ptr<Scope>> &active_scopes,
      const ast::AnonymousFunction &function
  );
  L3Function(
      std::vector<std::shared_ptr<Scope>> &&active_scopes,
      ast::FunctionBody body,
      std::optional<ast::Identifier> name
  );
  ~L3Function();

  RefValue operator()(VM &vm, L3Args args);

  DEFINE_ACCESSOR(body, ast::FunctionBody, body)
  [[nodiscard]] const ast::Identifier &get_name() const;

private:
  std::vector<std::shared_ptr<Scope>> capture_scopes;
  ast::FunctionBody body;
  std::optional<ast::Identifier> name;

  static ast::Identifier anonymous_function_name;
};

class BuiltinFunction {
public:
  using Body = std::function<RefValue(VM &vm, L3Args args)>;
  BuiltinFunction(ast::Identifier &&name, Body &&body);

  RefValue operator()(VM &vm, std::span<RefValue> args) const;

  DEFINE_ACCESSOR(name, ast::Identifier, name)
  DEFINE_ACCESSOR(body, Body, body)

private:
  ast::Identifier name;
  Body body;
};

class Function {
public:
  Function(L3Function &&function);
  Function(BuiltinFunction &&function);

  RefValue operator()(VM &vm, L3Args args);

  auto visit(auto &&...visitor) const {
    return match::match(inner, std::forward<decltype(visitor)>(visitor)...);
  }

private:
  std::variant<L3Function, BuiltinFunction> inner;
};

} // namespace l3::vm
