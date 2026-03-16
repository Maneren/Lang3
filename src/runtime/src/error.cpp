module l3.runtime;

import l3.ast;
import l3.location;
import :identifier;

namespace l3::runtime {

UndefinedVariableError::UndefinedVariableError(const Identifier &id)
    : NameError("variable '{}' not declared", id.get_name()) {}

RuntimeError::RuntimeError(const std::string &message)
    : std::runtime_error(message) {}

RuntimeError::RuntimeError(
    const std::string &message, const location::Location &location
)
    : std::runtime_error(message), location_(location) {}

std::string RuntimeError::format_error() const {
  std::string result;

  // Format: "ErrorType: message"
  result += std::format("{}: {}", type(), what());

  // Add location if available
  if (location_) {
    result += std::format("\n  at {}", location_->get());
  }

  return result;
}

} // namespace l3::runtime
