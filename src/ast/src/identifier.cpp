module l3.ast;

namespace l3::ast {

Identifier::Identifier() = default;
Identifier::Identifier(std::string &&name, location::Location location)
    : name(std::move(name)), location_(location) {}
Identifier::Identifier(std::string_view name, location::Location location)
    : name(name), location_(location) {}
Identifier::Identifier(const char *name, location::Location location)
    : name(name), location_(location) {}

} // namespace l3::ast
