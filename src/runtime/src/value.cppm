export module l3.runtime:value;

import utils;
import :primitive;
import :function;
import :ref_value;

export namespace l3::runtime {

struct Nil {
  std::strong_ordering operator<=>(Nil /*unused*/) const {
    return std::strong_ordering::equal;
  };
};

using L3Args = std::span<const Value>;

using NewValue = std::variant<Ref, Value>;

struct Slice {
  std::optional<std::int64_t> start, end;
};

class Value;

class HeapValue {
  std::variant<
      Nil,
      std::shared_ptr<Function>,
      std::shared_ptr<std::vector<Ref>>,
      std::string>
      inner;

public:
  template <typename T>
    requires(!std::is_same_v<T, HeapValue>)
  HeapValue(T &&t) : inner(std::forward<T>(t)) {}

  VISIT(inner)
  DEFINE_ACCESSOR_X(inner);
};

class Value {
public:
  using function_type = std::shared_ptr<Function>;
  using vector_type = std::shared_ptr<std::vector<Ref>>;
  using string_type = std::string;

private:
  std::variant<Primitive, std::shared_ptr<HeapValue>> inner;

public:
  Value();

  Value(const Value &) = default;
  Value(Value &&) = default;
  Value &operator=(const Value &) = default;
  Value &operator=(Value &&) = default;
  ~Value() = default;

  Value(Nil /*unused*/);
  Value(Primitive primitive);
  Value(Function &&function);
  Value(const Function &function);
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

  auto visit(auto &&...visitor) -> decltype(auto) {
    return std::visit(
        match::Overloaded{
            [&](Primitive &p) -> decltype(auto) {
              return match::Overloaded{
                  std::forward<decltype(visitor)>(visitor)...
              }(p);
            },
            [&](std::shared_ptr<HeapValue> &h) -> decltype(auto) {
              return match::match(
                  h->get_inner_mut(),
                  std::forward<decltype(visitor)>(visitor)...
              );
            }
        },
        inner
    );
  }
  auto visit(auto &&...visitor) const -> decltype(auto) {
    return std::visit(
        match::Overloaded{
            [&](const Primitive &p) -> decltype(auto) {
              return match::Overloaded{
                  std::forward<decltype(visitor)>(visitor)...
              }(p);
            },
            [&](const std::shared_ptr<HeapValue> &h) -> decltype(auto) {
              return match::match(
                  h->get_inner(), std::forward<decltype(visitor)>(visitor)...
              );
            }
        },
        inner
    );
  }

  [[nodiscard]] bool is_nil() const;
  [[nodiscard]] bool is_function() const;
  [[nodiscard]] bool is_primitive() const;
  [[nodiscard]] bool is_vector() const;
  [[nodiscard]] bool is_string() const;

  [[nodiscard]] utils::optional_cref<Primitive> as_primitive() const;
  [[nodiscard]] utils::optional_cref<function_type> as_function() const;
  [[nodiscard]] utils::optional_ref<function_type> as_mut_function();
  [[nodiscard]] utils::optional_cref<vector_type> as_vector() const;
  [[nodiscard]] utils::optional_ref<vector_type> as_mut_vector();
  [[nodiscard]] utils::optional_cref<string_type> as_string() const;
  [[nodiscard]] utils::optional_ref<string_type> as_mut_string();

  [[nodiscard]] bool is_truthy() const;
  [[nodiscard]] bool is_falsy() const { return !is_truthy(); }

  [[nodiscard]] NewValue index(const Value &index) const;
  [[nodiscard]] NewValue index(std::size_t index) const;

  [[nodiscard]] Ref &index_mut(const Value &index);
  [[nodiscard]] Ref &index_mut(std::size_t index);

  [[nodiscard]] Value slice(Slice slice) const;

  [[nodiscard]] std::string_view type_name() const;

  DEFINE_ACCESSOR_X(inner)
};

} // namespace l3::runtime
