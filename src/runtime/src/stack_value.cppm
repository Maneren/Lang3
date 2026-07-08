export module l3.runtime:stack_value;

import std;
import utils;
import :primitive;

export namespace l3::runtime {

class GCValue;
class Value;

struct Slice {
  std::optional<std::int64_t> start, end;
};

struct Nil {
  std::strong_ordering operator<=>(Nil /*unused*/) const {
    return std::strong_ordering::equal;
  };
};

class StackValue {
  std::variant<Nil, Primitive, GCValue *> inner;

public:
  StackValue();
  StackValue(Nil);
  StackValue(Primitive primitive);
  StackValue(GCValue *gc_value);

  StackValue(const StackValue &) = default;
  StackValue(StackValue &&) = default;
  StackValue &operator=(const StackValue &) = default;
  StackValue &operator=(StackValue &&) = default;
  ~StackValue() = default;

  auto visit(this auto &&self, auto &&...visitor) -> decltype(auto) {
    return std::visit(
        match::Overloaded{std::forward<decltype(visitor)>(visitor)...},
        self.inner
    );
  }

  [[nodiscard]] bool is_nil() const;
  [[nodiscard]] bool is_primitive() const;

  [[nodiscard]] bool is_truthy() const;

  [[nodiscard]] std::string_view type_name() const;

  [[nodiscard]] utils::optional_cref<Primitive> as_primitive() const;

  [[nodiscard]] bool holds_gc_value() const {
    return std::holds_alternative<GCValue *>(inner);
  }

  [[nodiscard]] GCValue *get_gc_ptr() const noexcept {
    if (const auto *ptr = std::get_if<GCValue *>(&inner)) {
      return *ptr;
    }
    return nullptr;
  }

  GCValue *&get_gc_ptr_mut() { return std::get<GCValue *>(inner); }

  [[nodiscard]] bool is_string() const;
  [[nodiscard]] bool is_vector() const;
  [[nodiscard]] bool is_function() const;

  [[nodiscard]] utils::optional_cref<std::string> as_string() const;
  [[nodiscard]] utils::optional_cref<std::vector<StackValue>> as_vector() const;
  [[nodiscard]] utils::optional_ref<std::vector<StackValue>> as_mut_vector();

  [[nodiscard]] Value slice(Slice slice) const;

  DEFINE_ACCESSOR_X(inner)
};

} // namespace l3::runtime
