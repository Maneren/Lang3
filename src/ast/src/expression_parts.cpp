module l3.ast;

namespace l3::ast {

Comparison::Type Comparison::get_type(ComparisonOperator op) {
  switch (op) {
  case ComparisonOperator::Equal:
  case ComparisonOperator::NotEqual:
    return Type::Equality;
  default:
    return Type::Inequality;
  }
}

} // namespace l3::ast
