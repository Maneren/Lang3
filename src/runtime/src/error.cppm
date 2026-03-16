export module l3.runtime:error;

import :identifier;
import l3.location;
import std;
import utils;

export namespace l3::runtime {

class RuntimeError : public std::runtime_error {
  utils::optional_cref<location::Location> location_;

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
        location_(location) {}

  [[nodiscard]] constexpr virtual std::string_view type() const {
    return "RuntimeError";
  }

  [[nodiscard]] utils::optional_cref<const location::Location>
  location() const {
    return location_;
  }

  void set_location(const location::Location &location) {
    if (!location_) {
      location_ = location;
    }
  }

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

  UndefinedVariableError(const Identifier &id);

  [[nodiscard]] constexpr std::string_view type() const override {
    return "UndefinedVariableError";
  }
};

} // namespace l3::runtime
