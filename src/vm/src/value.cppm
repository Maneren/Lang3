module;

#include <memory>
#include <optional>
#include <span>
#include <utils/accessor.h>
#include <utils/visit.h>
#include <variant>
#include <vector>

export module l3.vm:value;

import utils;
import :primitive;
import :function;
import :ref_value;

export namespace l3::vm {

// Abstraction over vector and string
template <typename T>
concept ValueContainer = requires(T &container) {
  { container.size() } -> std::convertible_to<std::size_t>;
  { container.empty() } -> std::convertible_to<bool>;
  { container.reserve(std::size_t{}) } -> std::convertible_to<void>;
};

struct Nil {
  std::strong_ordering operator<=>(Nil /*unused*/) const {
    return std::strong_ordering::equal;
  };
};

using L3Args = std::span<const RefValue>;

using NewValue = std::variant<RefValue, Value>;

struct Slice {
  std::optional<std::int64_t> start, end;
};

class Value {
public:
  using function_type = Function;
  using vector_type = std::vector<RefValue>;
  using string_type = std::string;

private:
  std::variant<Nil, Primitive, function_type, vector_type, string_type> inner;

public:
  Value();

  Value(const Value &) = delete;
  Value(Value &&) = default;
  Value &operator=(const Value &) = delete;
  Value &operator=(Value &&) = default;
  ~Value() = default;

  Value(Nil /*unused*/);
  Value(Primitive &&primitive);
  Value(Function &&function);
  Value(vector_type &&vector);
  Value(string_type &&string);

  [[nodiscard]] Value add(const Value &other) const;
  void add_assign(const Value &other);
  [[nodiscard]] Value sub(const Value &other) const;
  [[nodiscard]] Value mul(const Value &other) const;
  void mul_assign(const Value &other);
  [[nodiscard]] Value div(const Value &other) const;
  [[nodiscard]] Value mod(const Value &other) const;
  [[nodiscard]] std::partial_ordering compare(const Value &other) const;

  [[nodiscard]] Value not_op() const;
  [[nodiscard]] Value negative() const;

  VISIT(inner);

  [[nodiscard]] bool is_nil() const;
  [[nodiscard]] bool is_function() const;
  [[nodiscard]] bool is_primitive() const;
  [[nodiscard]] bool is_vector() const;
  [[nodiscard]] bool is_string() const;

  [[nodiscard]] utils::optional_cref<Primitive> as_primitive() const;
  [[nodiscard]] utils::optional_cref<function_type> as_function() const;
  [[nodiscard]] utils::optional_cref<vector_type> as_vector() const;
  [[nodiscard]] utils::optional_ref<vector_type> as_mut_vector();
  [[nodiscard]] utils::optional_cref<string_type> as_string() const;
  [[nodiscard]] utils::optional_ref<string_type> as_mut_string();

  [[nodiscard]] bool is_truthy() const;
  [[nodiscard]] bool is_falsy() const { return !is_truthy(); }

  [[nodiscard]] NewValue index(const Value &index) const;
  [[nodiscard]] NewValue index(size_t index) const;

  [[nodiscard]] RefValue &index_mut(const Value &index);
  [[nodiscard]] RefValue &index_mut(size_t index);

  [[nodiscard]] Value slice(Slice slice) const;

  [[nodiscard]] std::string_view type_name() const;

  DEFINE_ACCESSOR_X(inner)
};

} // namespace l3::vm
