module l3.ast;

import std;

namespace l3::ast {

Declaration::Declaration(location::Location location)
    : location_(std::move(location)) {}
Declaration::Declaration(
    NameList &&names,
    std::optional<Expression> &&expression,
    Mutability mutability,
    location::Location location
)
    : names(std::move(names)), expression(std::move(expression)),
      mutability(mutability), location_(std::move(location)) {}

} // namespace l3::ast
