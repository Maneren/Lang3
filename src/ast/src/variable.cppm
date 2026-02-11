export module l3.ast:variable;

import :identifier;
import std;
import l3.location;

export namespace l3::ast {

class Variable;
class Expression;

class IndexExpression {
  std::unique_ptr<Variable> base;
  std::unique_ptr<Expression> index;

  DEFINE_LOCATION_FIELD()

public:
  IndexExpression(location::Location location = {});
  IndexExpression(
      Variable &&base, Expression &&index, location::Location location = {}
  );

  IndexExpression(const IndexExpression &) = delete;
  IndexExpression(IndexExpression &&) noexcept;
  IndexExpression &operator=(const IndexExpression &) = delete;
  IndexExpression &operator=(IndexExpression &&) noexcept;
  ~IndexExpression();

  DEFINE_PTR_ACCESSOR_X(base)
  DEFINE_PTR_ACCESSOR_X(index)
};

class Variable {
  std::variant<Identifier, IndexExpression> inner;

public:
  Variable();
  Variable(Identifier &&id);
  Variable(IndexExpression &&ie);

  VISIT(inner);

  [[nodiscard]] const location::Location &get_location() const;
};

} // namespace l3::ast
