module l3.ast;

namespace l3::ast {

Declaration::Declaration() = default;
Declaration::Declaration(
    NameList &&names,
    std::optional<Expression> &&expression,
    Mutability mutability
)
    : names(std::move(names)), expression(std::move(expression)),
      mutability(mutability) {}

} // namespace l3::ast
