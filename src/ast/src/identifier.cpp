module l3.ast;

namespace l3::ast {

Identifier::Identifier() = default;
Identifier::Identifier(std::string &&name, location::Location location)
    : name(std::move(name)), location_(std::move(location)) {}
Identifier::Identifier(std::string_view name, location::Location location)
    : name(name), location_(std::move(location)) {}

} // namespace l3::ast
