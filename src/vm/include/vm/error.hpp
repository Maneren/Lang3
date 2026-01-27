#pragma once

#include <ast/ast.hpp>
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

  [[nodiscard]] constexpr virtual std::string_view type() const {
    return "RuntimeError";
  }
};

class UnsupportedOperation : public RuntimeError {
public:
  using RuntimeError::RuntimeError;

  template <typename T, typename U>
  UnsupportedOperation(std::string_view operation, T lhs, U rhs)
      : RuntimeError(
            "{} between '{}' and '{}' not supported", operation, lhs, rhs
        ) {}

  template <typename T>
  UnsupportedOperation(std::string_view operation, T value)
      : RuntimeError("unary {} of {} not supported", operation, value) {}

  [[nodiscard]] constexpr std::string_view type() const override {
    return "UnsupportedOperation";
  }
};

class ValueError : public RuntimeError {
public:
  using RuntimeError::RuntimeError;

  [[nodiscard]] constexpr std::string_view type() const override {
    return "ValueError";
  }
};

class TypeError : public RuntimeError {
public:
  using RuntimeError::RuntimeError;

  [[nodiscard]] constexpr std::string_view type() const override {
    return "TypeError";
  }
};

class NameError : public RuntimeError {
public:
  using RuntimeError::RuntimeError;

  [[nodiscard]] constexpr std::string_view type() const override {
    return "NameError";
  }
};

class UndefinedVariableError : public NameError {
public:
  using NameError::NameError;

  UndefinedVariableError(const ast::Identifier &id);
  UndefinedVariableError(const ast::Variable &id);

  [[nodiscard]] constexpr std::string_view type() const override {
    return "UndefinedVariableError";
  }
};

} // namespace l3::vm
