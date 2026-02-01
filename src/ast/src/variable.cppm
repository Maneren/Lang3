module;

#include <memory>
#include <utils/accessor.h>
#include <utils/visit.h>
#include <variant>

export module l3.ast:variable;

import :identifier;

export namespace l3::ast {

class Variable;
class Expression;

class IndexExpression {
  std::unique_ptr<Variable> base;
  std::unique_ptr<Expression> index;

public:
  IndexExpression();
  IndexExpression(Variable &&base, Expression &&index);

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
  Variable() = default;
  Variable(Identifier &&id) : inner(std::move(id)) {}
  Variable(IndexExpression &&ie) : inner(std::move(ie)) {}

  VISIT(inner);
};

} // namespace l3::ast
