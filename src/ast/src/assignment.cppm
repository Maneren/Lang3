module;

#include <variant>

export module l3.ast:assignment;

export import :operator_assignment;
export import :name_assignment;

export namespace l3::ast {

using Assignment = std::variant<OperatorAssignment, NameAssignment>;

} // namespace l3::ast
