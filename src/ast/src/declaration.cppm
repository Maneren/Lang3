module;

#include <iterator>
#include <optional>
#include <utility>
#include <utils/accessor.h>

export module l3.ast:declaration;

import :expression;
import :mutability;
import :name_list;

export namespace l3::ast {

class Declaration {
  NameList names;
  std::optional<Expression> expression;
  Mutability mutability = Mutability::Immutable;

public:
  Declaration() = default;
  Declaration(
      NameList &&names,
      std::optional<Expression> &&expression,
      Mutability mutability
  )
      : names(std::move(names)), expression(std::move(expression)),
        mutability(mutability) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Declaration ({})", mutability);
    format_indented_line(out, depth + 1, "Names");
    get_names().print(out, depth + 2);
    format_indented_line(out, depth + 1, "Expression");
    if (expression.has_value()) {
      get_expression()->print(out, depth + 2);
    } else {
      format_indented_line(out, depth + 2, "nil (implicit)");
    }
  }

  DEFINE_ACCESSOR_X(names);
  DEFINE_ACCESSOR_X(expression);
  DEFINE_VALUE_ACCESSOR_X(mutability);

  [[nodiscard]] bool is_const() const {
    return mutability == Mutability::Immutable;
  }
  [[nodiscard]] bool is_mutable() const {
    return mutability == Mutability::Mutable;
  }
};

} // namespace l3::ast
