#pragma once

#include "utils/accessor.h"
#include "vm/identifier.hpp"
#include "vm/value.hpp"
#include <memory>
#include <optional>
#include <span>
#include <vector>

import l3.ast;

namespace l3::vm {

class Scope;
class VM;

using L3Args = std::span<RefValue>;

class L3Function {
  std::vector<std::shared_ptr<Scope>> capture_scopes;
  ast::FunctionBody body;
  std::optional<Identifier> name;

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
      std::optional<Identifier> name
  );
  ~L3Function();

  RefValue operator()(VM &vm, L3Args args);

  DEFINE_ACCESSOR_X(body);
  [[nodiscard]] const Identifier &get_name() const;

private:
  static Identifier anonymous_function_name;
};

class BuiltinFunction {
public:
  using Body = std::function<RefValue(VM &vm, L3Args args)>;

private:
  Identifier name;
  Body body;

public:
  BuiltinFunction(Identifier &&name, Body &&body);

  RefValue operator()(VM &vm, std::span<RefValue> args) const;

  DEFINE_ACCESSOR_X(name);
  DEFINE_ACCESSOR_X(body);
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
