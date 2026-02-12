module l3.vm;

import l3.ast;
import l3.location;
import :identifier;

namespace l3::vm {

UndefinedVariableError::UndefinedVariableError(const Identifier &id)
    : NameError("variable '{}' not declared", id.get_name()) {}

RuntimeError::RuntimeError(const std::string &message)
    : std::runtime_error(message) {}

RuntimeError::RuntimeError(
    const std::string &message,
    const location::Location &location,
    std::vector<CallStackFrame> stack_trace
)
    : std::runtime_error(message), location_(location),
      stack_trace_(std::move(stack_trace)) {}

std::string RuntimeError::format_error() const {
  std::string result;

  // Format: "ErrorType: message"
  result += std::format("{}: {}", type(), what());

  // Add location if available
  if (location_) {
    result += std::format("\n  at {}", location_->get());
  }

  // Add stack trace
  if (!stack_trace_.empty()) {
    result += "\n\nStack trace:";
    for (const auto &frame : stack_trace_) {
      result +=
          std::format("\n  in {} at {}", frame.function_name, frame.location);
    }
  }

  return result;
}

} // namespace l3::vm
