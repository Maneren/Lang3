#include "vm/error.hpp"
#include <ast/ast.hpp>
#include <stdexcept>
#include <string>

namespace l3::vm {

UndefinedVariableError::UndefinedVariableError(const ast::Identifier &id)
    : NameError("variable '{}' not declared", id.get_name()) {}

UndefinedVariableError::UndefinedVariableError(const ast::Variable &id)
    : NameError("variable '{}' not declared", id.get_identifier().get_name()) {}

RuntimeError::RuntimeError(const std::string &message)
    : std::runtime_error(message) {}

} // namespace l3::vm
