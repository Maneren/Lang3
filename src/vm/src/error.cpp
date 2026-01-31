module;

#include <stdexcept>
#include <string>

module l3.vm;

import :identifier;

namespace l3::vm {

UndefinedVariableError::UndefinedVariableError(const Identifier &id)
    : NameError("variable '{}' not declared", id.get_name()) {}

RuntimeError::RuntimeError(const std::string &message)
    : std::runtime_error(message) {}

} // namespace l3::vm
