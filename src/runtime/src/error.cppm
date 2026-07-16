export module l3.runtime:error;

import std;

import utils;

import l3.ast;
import l3.location;

export namespace l3::runtime {

struct StacktraceFrame {
  std::string function_name;
  location::Location call_location;
};

class RuntimeError : public std::runtime_error {
  utils::optional_cref<location::Location> location;
  std::vector<StacktraceFrame> stacktrace;
  mutable std::string formatted;

public:
  RuntimeError(const std::string &message);
  RuntimeError(const std::string &message, const location::Location &location);

  template <typename... Args>
  RuntimeError(const std::format_string<Args...> &message, Args &&...args)
      : std::runtime_error(std::format(message, std::forward<Args>(args)...)) {}

  template <typename... Args>
  RuntimeError(
      const location::Location &location,
      const std::format_string<Args...> &message,
      Args &&...args
  )
      : std::runtime_error(std::format(message, std::forward<Args>(args)...)),
        location(location) {}

  [[nodiscard]] constexpr virtual std::string_view type() const {
    return "RuntimeError";
  }

  [[nodiscard]] utils::optional_cref<const location::Location>
  get_location() const {
    return location;
  }

  void set_location(const location::Location &location);
  void set_stacktrace(std::vector<StacktraceFrame> stacktrace);

  [[nodiscard]] const char *what() const noexcept override;

  [[nodiscard]] std::string format_error() const;
};

class UnsupportedOperation : public RuntimeError {
public:
  using RuntimeError::RuntimeError;

  UnsupportedOperation(std::string_view operation, auto lhs, auto rhs)
      : RuntimeError(
            "{} between '{}' and '{}' not supported", operation, lhs, rhs
        ) {}

  UnsupportedOperation(std::string_view operation, auto value)
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

  [[nodiscard]] constexpr std::string_view type() const override {
    return "UndefinedVariableError";
  }
};

} // namespace l3::runtime
