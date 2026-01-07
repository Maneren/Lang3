#pragma once

#include "ast/nodes/identifier.hpp"
#include <format>
#include <stdexcept>
#include <string>

namespace l3::vm {

class RuntimeError : public std::runtime_error {
public:
  RuntimeError(const std::string &message);

  template <typename... Args>
  RuntimeError(const std::format_string<Args...> &message, Args &&...args)
      : std::runtime_error(std::format(message, std::forward<Args>(args)...)) {}
};

class UnsupportedOperation : public RuntimeError {
public:
  using RuntimeError::RuntimeError;

  template <typename T, typename U>
  UnsupportedOperation(std::string_view operation, T lhs, U rhs)
      : RuntimeError(
            "{} between '{}' and '{}' not supported", operation, lhs, rhs
        ) {}
};

class NameError : public RuntimeError {
public:
  using RuntimeError::RuntimeError;
};

class UndefinedVariableError : public NameError {
public:
  using NameError::NameError;

  UndefinedVariableError(const ast::Identifier &id);
  UndefinedVariableError(const ast::Variable &id);
};

} // namespace l3::vm
