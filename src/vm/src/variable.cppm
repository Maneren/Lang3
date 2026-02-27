export module l3.vm:variable;

import l3.ast;
import :ref_value;
import :mutability;

export namespace l3::vm {

class Variable {
  Ref ref_value;
  Mutability mutability;

public:
  Variable() = delete;
  Variable(Ref ref_value, Mutability mutability);

  [[nodiscard]] const Ref &get() const { return ref_value; }
  [[nodiscard]] Ref &get() { return ref_value; }

  [[nodiscard]] Ref &operator*() { return get(); }
  [[nodiscard]] const Ref &operator*() const { return get(); }

  Ref *operator->() { return &get(); }
  const Ref *operator->() const { return &get(); }

  DEFINE_VALUE_ACCESSOR_X(mutability);

  [[nodiscard]] bool is_const() const {
    return mutability == Mutability::Immutable;
  }
  [[nodiscard]] bool is_mutable() const {
    return mutability == Mutability::Mutable;
  }
};

} // namespace l3::vm
