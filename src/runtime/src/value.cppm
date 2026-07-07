export module l3.runtime:value;

import utils;
import :primitive;
import :function;
import :stack_value;

export namespace l3::runtime {

using NewValue = std::variant<StackValue, Value>;

struct Slice {
  std::optional<std::int64_t> start, end;
};

class Value {
public:
  using function_type = std::unique_ptr<Function>;
  using vector_type = std::vector<StackValue>;
  using string_type = std::string;

private:
  std::variant<
      Nil,
      Primitive,
      std::unique_ptr<Function>,
      vector_type,
      string_type>
      inner;

  using variant = decltype(inner);

public:
  Value();

  Value(const Value &other);
  Value(Value &&) = default;
  Value &operator=(const Value &other);
  Value &operator=(Value &&) = default;
  ~Value() = default;

  Value(Nil);
  Value(Primitive primitive);
  Value(std::unique_ptr<Function> &&function);
  Value(const Function &function);
  Value(Function &&function);
  Value(vector_type &&vector);
  Value(string_type &&string);

  [[nodiscard]] Value add(const Value &other) const;
  void add_assign(const Value &other);
  [[nodiscard]] Value sub(const Value &other) const;
  [[nodiscard]] Value mul(const Value &other) const;
  [[nodiscard]] Value div(const Value &other) const;
  [[nodiscard]] Value mod(const Value &other) const;
  [[nodiscard]] std::partial_ordering compare(const Value &other) const;

  [[nodiscard]] Value not_op() const;
  [[nodiscard]] Value negative() const;

  auto visit(auto &&...visitor) -> decltype(auto) {
    return match::match(inner, std::forward<decltype(visitor)>(visitor)...);
  }
  auto visit(auto &&...visitor) const -> decltype(auto) {
    return match::match(inner, std::forward<decltype(visitor)>(visitor)...);
  }

  [[nodiscard]] bool is_nil() const;
  [[nodiscard]] bool is_function() const;
  [[nodiscard]] bool is_primitive() const;
  [[nodiscard]] bool is_vector() const;
  [[nodiscard]] bool is_string() const;

  [[nodiscard]] utils::optional_cref<Primitive> as_primitive() const;
  [[nodiscard]] utils::optional_cref<vector_type> as_vector() const;
  [[nodiscard]] utils::optional_ref<vector_type> as_mut_vector();
  [[nodiscard]] utils::optional_cref<string_type> as_string() const;
  [[nodiscard]] utils::optional_ref<string_type> as_mut_string();

  [[nodiscard]] bool is_truthy() const;

  [[nodiscard]] Value slice(Slice slice) const;

  [[nodiscard]] std::string_view type_name() const;

  DEFINE_ACCESSOR_X(inner)
};

// Operations on StackValues directly (no value_from_stack round trip)
// These extract references to GC data without copying, perform the op, and
// return a new Value.

[[nodiscard]] Value to_value(const StackValue &sv);
[[nodiscard]] StackValue &
index_mut(StackValue &container, const StackValue &index);

// Arithmetic binary ops
[[nodiscard]] Value add(const StackValue &a, const StackValue &b);
[[nodiscard]] Value sub(const StackValue &a, const StackValue &b);
[[nodiscard]] Value mul(const StackValue &a, const StackValue &b);
[[nodiscard]] Value div(const StackValue &a, const StackValue &b);
[[nodiscard]] Value mod(const StackValue &a, const StackValue &b);

// Comparison
[[nodiscard]] std::partial_ordering
compare(const StackValue &a, const StackValue &b);

// Unary ops
[[nodiscard]] Value negative(const StackValue &sv);
[[nodiscard]] Value not_op(const StackValue &sv);

// Indexing
[[nodiscard]] NewValue
index(const StackValue &container, const StackValue &index);

} // namespace l3::runtime
