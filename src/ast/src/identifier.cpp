module l3.ast;

namespace l3::ast {

Identifier::Identifier() = default;
Identifier::Identifier(std::string &&name) : name(std::move(name)) {}
Identifier::Identifier(std::string_view name) : name(name) {}

} // namespace l3::ast
