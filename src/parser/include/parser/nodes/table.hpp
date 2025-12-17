#pragma once

#include "expression.hpp"

#include <memory>
#include <variant>
#include <vector>

namespace l3::ast {

using FieldName = std::variant<Identifier, std::shared_ptr<Expression>>;

struct AssignmentField {
  FieldName name;
  std::shared_ptr<Expression> expr;
};

using Field = std::variant<AssignmentField, Expression>;

struct FieldList {
  std::vector<Field> fields;
};

struct Table {
  FieldList fields;
};

} // namespace l3::ast
