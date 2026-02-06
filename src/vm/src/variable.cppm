export module l3.vm:variable;

import l3.ast;
import :ref_value;
import :mutability;

export namespace l3::vm {

class Variable {
  RefValue ref_value;
  Mutability mutability;

public:
  Variable() = delete;
  Variable(RefValue ref_value, Mutability mutability);

  [[nodiscard]] const RefValue &get() const { return ref_value; }
  [[nodiscard]] RefValue &get() { return ref_value; }

  [[nodiscard]] RefValue &operator*() { return get(); }
  [[nodiscard]] const RefValue &operator*() const { return get(); }

  RefValue *operator->() { return &get(); }
  const RefValue *operator->() const { return &get(); }

  DEFINE_VALUE_ACCESSOR_X(mutability);

  [[nodiscard]] bool is_const() const {
    return mutability == Mutability::Immutable;
  }
  [[nodiscard]] bool is_mutable() const {
    return mutability == Mutability::Mutable;
  }
};

} // namespace l3::vm
