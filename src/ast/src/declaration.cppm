module;

#include <iterator>
#include <utility>
#include <utils/accessor.h>

export module l3.ast:declaration;

import :mutability;
import :name_assignment;

export namespace l3::ast {

class Declaration {
  NameAssignment name_assignment;
  Mutability mutability = Mutability::Immutable;

public:
  Declaration() = default;
  Declaration(NameAssignment &&name_assignment, Mutability mutability)
      : name_assignment(std::move(name_assignment)), mutability(mutability) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Declaration ({})", mutability);
    get_name_assignment().print(out, depth + 1);
  }

  DEFINE_ACCESSOR_X(name_assignment);
  DEFINE_VALUE_ACCESSOR_X(mutability);

  [[nodiscard]] bool is_const() const {
    return mutability == Mutability::Immutable;
  }
  [[nodiscard]] bool is_mutable() const {
    return mutability == Mutability::Mutable;
  }
};

} // namespace l3::ast
