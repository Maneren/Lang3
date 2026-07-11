module l3.runtime;

import l3.ast;
import l3.location;
namespace l3::runtime {

UndefinedVariableError::UndefinedVariableError(const ast::Identifier &id)
    : NameError("variable '{}' not declared", id.get_name()) {}

RuntimeError::RuntimeError(const std::string &message)
    : std::runtime_error(message) {}

RuntimeError::RuntimeError(
    const std::string &message, const location::Location &location
)
    : std::runtime_error(message), location(location) {}

std::string RuntimeError::format_error() const {
  std::string result;

  // Format: "ErrorType: message"
  result += std::format("{}: {}", type(), std::runtime_error::what());

  if (location) {
    result += std::format("\n  at {}", *location);
  }

  if (!stacktrace.empty()) {
    result += "\nstacktrace:";
    for (const auto &frame : std::views::reverse(stacktrace)) {
      result += std::format(
          "\n  in {} called at {}", frame.function_name, frame.call_location
      );
    }
  }

  return result;
}

const char *RuntimeError::what() const noexcept { return formatted.c_str(); }

void RuntimeError::set_location(const location::Location &loc) {
  location = loc;
  formatted = format_error();
}

void RuntimeError::set_stacktrace(std::vector<StacktraceFrame> st) {
  stacktrace = std::move(st);
  formatted = format_error();
}

} // namespace l3::runtime
