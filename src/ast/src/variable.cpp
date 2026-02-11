module l3.ast;

namespace l3::ast {

IndexExpression::IndexExpression(location::Location location)
    : location_(std::move(location)) {}
IndexExpression::IndexExpression(
    Variable &&base, Expression &&index, location::Location location
)
    : base(std::make_unique<Variable>(std::move(base))),
      index(std::make_unique<Expression>(std::move(index))),
      location_(std::move(location)) {};

IndexExpression::IndexExpression(IndexExpression &&) noexcept = default;
IndexExpression &
IndexExpression::operator=(IndexExpression &&) noexcept = default;
IndexExpression::~IndexExpression() = default;

Variable::Variable() = default;
Variable::Variable(Identifier &&id) : inner(std::move(id)) {}
Variable::Variable(IndexExpression &&ie) : inner(std::move(ie)) {}

const location::Location &Variable::get_location() const {
  return std::visit(
      [](const auto &node) -> const location::Location & {
        return node.get_location();
      },
      inner
  );
}

} // namespace l3::ast
