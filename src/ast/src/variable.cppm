module;

#include <iterator>
#include <memory>
#include <utils/accessor.h>
#include <utils/match.h>
#include <variant>

export module l3.ast:variable;

import :identifier;
import :printing;

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

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_PTR_ACCESSOR_X(base)
  DEFINE_PTR_ACCESSOR_X(index)
};

class Variable {
  std::variant<Identifier, IndexExpression> inner;

public:
  Variable() = default;
  Variable(Identifier &&id) : inner(std::move(id)) {}
  Variable(IndexExpression &&ie) : inner(std::move(ie)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Variable");
    visit([&out, depth](auto &inner) { inner.print(out, depth + 1); });
  }

  VISIT(inner);
};

} // namespace l3::ast
