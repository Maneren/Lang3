#pragma once

#include "ast/nodes/function.hpp"
#include "vm/value.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace l3::vm {

class Scope;
class VM;

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

  CowValue operator()(VM &vm, std::span<const CowValue> args);

  [[nodiscard]] const ast::Identifier &get_name() const;

private:
  std::vector<std::shared_ptr<Scope>> capture_scopes;
  ast::FunctionBody body;
  std::optional<ast::Identifier> name;

  static ast::Identifier anonymous_function_name;
};

class BuiltinFunction {
public:
  using Body = std::function<CowValue(VM &vm, std::span<const CowValue> args)>;
  BuiltinFunction(ast::Identifier &&name, Body &&body)
      : name{std::move(name)}, body{std::move(body)} {}

  CowValue operator()(VM &vm, std::span<const CowValue> args) const {
    return body(vm, args);
  }

  [[nodiscard]] const ast::Identifier &get_name() const { return name; }

private:
  ast::Identifier name;
  Body body;
};

class Function {
public:
  Function(L3Function &&function);
  Function(BuiltinFunction &&function);

  CowValue operator()(VM &vm, std::span<const CowValue> args);

  auto visit(auto &&...visitor) const {
    return match::match(inner, std::forward<decltype(visitor)>(visitor)...);
  }

private:
  std::variant<L3Function, BuiltinFunction> inner;
};

} // namespace l3::vm
