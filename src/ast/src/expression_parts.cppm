export module l3.ast:expression_parts;

import :identifier;
import :operators;
import l3.location;

export namespace l3::ast {

class Expression;

class UnaryExpression {
  UnaryOperator op = UnaryOperator::Plus;
  std::unique_ptr<Expression> expression;

  DEFINE_LOCATION_FIELD()

public:
  UnaryExpression() = default;
  UnaryExpression(
      UnaryOperator op,
      Expression &&expression,
      location::Location location = {}
  );

  DEFINE_VALUE_ACCESSOR_X(op)
  DEFINE_PTR_ACCESSOR_X(expression)
};

class BinaryExpression {
  std::unique_ptr<Expression> lhs;
  BinaryOperator op = BinaryOperator::Plus;
  std::unique_ptr<Expression> rhs;

  DEFINE_LOCATION_FIELD()

public:
  BinaryExpression() = default;
  BinaryExpression(
      Expression &&lhs,
      BinaryOperator op,
      Expression &&rhs,
      location::Location location = {}
  );

  DEFINE_PTR_ACCESSOR_X(lhs)
  DEFINE_VALUE_ACCESSOR_X(op)
  DEFINE_PTR_ACCESSOR_X(rhs)
};

class LogicalExpression {
  std::unique_ptr<Expression> lhs;
  LogicalOperator op = LogicalOperator::And;
  std::unique_ptr<Expression> rhs;

  DEFINE_LOCATION_FIELD()

public:
  LogicalExpression() = default;
  LogicalExpression(
      Expression &&lhs,
      LogicalOperator op,
      Expression &&rhs,
      location::Location location = {}
  );

  DEFINE_PTR_ACCESSOR_X(lhs)
  DEFINE_VALUE_ACCESSOR_X(op)
  DEFINE_PTR_ACCESSOR_X(rhs)
};

class Comparison {
  enum class Type : std::uint8_t { Equality, Inequality };

  constexpr static Type get_type(ComparisonOperator op) {
    switch (op) {
    case ComparisonOperator::Equal:
    case ComparisonOperator::NotEqual:
      return Type::Equality;
    default:
      return Type::Inequality;
    }
  }

  std::unique_ptr<Expression> start;
  std::vector<std::pair<ComparisonOperator, Expression>> comparisons;
  Type type = Type::Equality;

  DEFINE_LOCATION_FIELD()

public:
  Comparison() = default;
  Comparison(
      Expression &&left,
      ComparisonOperator op,
      Expression &&right,
      location::Location location = {}
  );
  bool add_comparison(ComparisonOperator op, Expression &&right);

  DEFINE_PTR_ACCESSOR_X(start)
  DEFINE_ACCESSOR_X(comparisons)
};

} // namespace l3::ast
