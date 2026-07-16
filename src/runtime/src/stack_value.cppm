export module l3.runtime:stack_value;

import std;

import utils;

import :primitive;

export namespace l3::runtime {

class HeapCell;
class HeapData;

struct Slice {
  std::optional<std::int64_t> start, end;
};

struct Nil {
  std::strong_ordering operator<=>(Nil /*unused*/) const {
    return std::strong_ordering::equal;
  };
};

class StackValue {
  std::variant<Nil, Primitive, HeapCell *> inner;

public:
  StackValue();
  StackValue(Nil);
  StackValue(Primitive primitive);
  StackValue(HeapCell *gc_value);

  StackValue(const StackValue &) = default;
  StackValue(StackValue &&) = default;
  StackValue &operator=(const StackValue &) = default;
  StackValue &operator=(StackValue &&) = default;
  ~StackValue() = default;

  auto visit(this auto &&self, auto &&...visitor) -> decltype(auto) {
    return match::match(
        self.inner, std::forward<decltype(visitor)>(visitor)...
    );
  }

  [[nodiscard]] bool is_nil() const;
  [[nodiscard]] bool is_primitive() const;

  [[nodiscard]] bool is_truthy() const;

  [[nodiscard]] std::string_view type_name() const;

  [[nodiscard]] utils::optional_cref<Primitive> as_primitive() const;

  [[nodiscard]] bool holds_heap_cell() const {
    return std::holds_alternative<HeapCell *>(inner);
  }

  [[nodiscard]] constexpr auto get_heap_ptr(this auto &&self) noexcept
      -> std::conditional_t<
          std::is_const_v<std::remove_reference_t<decltype(self)>>,
          const HeapCell *,
          HeapCell *> {
    if (auto *ptr = std::get_if<HeapCell *>(&self.inner)) {
      return *ptr;
    }
    return nullptr;
  }

  [[nodiscard]] bool is_string() const;
  [[nodiscard]] bool is_vector() const;
  [[nodiscard]] bool is_function() const;

  [[nodiscard]] utils::optional_cref<std::string> as_string() const;
  [[nodiscard]] utils::optional_cref<std::vector<StackValue>> as_vector() const;
  [[nodiscard]] utils::optional_ref<std::vector<StackValue>> as_mut_vector();

  [[nodiscard]] HeapData slice(Slice slice) const;

  DEFINE_ACCESSOR_X(inner)
};

} // namespace l3::runtime
