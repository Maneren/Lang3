export module l3.runtime:heap_data;

import utils;
import :primitive;
import :function;
import :stack_value;

export namespace l3::runtime {

class HeapData {
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
  HeapData();

  HeapData(const HeapData &other);
  HeapData(HeapData &&) = default;
  HeapData &operator=(const HeapData &other);
  HeapData &operator=(HeapData &&) = default;
  ~HeapData() = default;

  HeapData(Nil);
  HeapData(Primitive primitive);
  HeapData(std::unique_ptr<Function> &&function);
  HeapData(const Function &function);
  HeapData(Function &&function);
  HeapData(vector_type &&vector);
  HeapData(string_type &&string);

  [[nodiscard]] HeapData add(const HeapData &other) const;
  void add_assign(const HeapData &other);
  [[nodiscard]] HeapData sub(const HeapData &other) const;
  [[nodiscard]] HeapData mul(const HeapData &other) const;
  [[nodiscard]] HeapData div(const HeapData &other) const;
  [[nodiscard]] HeapData mod(const HeapData &other) const;
  [[nodiscard]] HeapData pow(const HeapData &other) const;
  [[nodiscard]] std::partial_ordering compare(const HeapData &other) const;

  [[nodiscard]] HeapData not_op() const;
  [[nodiscard]] HeapData negative() const;

  auto visit(this auto &&self, auto &&...visitor) -> decltype(auto) {
    return match::match(
        self.inner,
        std::forward<decltype(visitor)>(visitor)...
    );
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

  [[nodiscard]] HeapData slice(Slice slice) const;

  [[nodiscard]] std::string_view type_name() const;

  DEFINE_ACCESSOR_X(inner)
};

// Operations on StackValues directly
// These extract references to GC data without copying, perform the op, and
// return a new HeapData.

[[nodiscard]] HeapData to_owned(const StackValue &sv);
[[nodiscard]] StackValue &
index_mut(StackValue &container, const StackValue &index);

// Indexing: returns StackValue directly (string substrings allocate on heap)
[[nodiscard]] StackValue
index(const StackValue &container, const StackValue &index, class Heap &heap);

// Arithmetic binary ops
[[nodiscard]] HeapData add(const StackValue &a, const StackValue &b);
[[nodiscard]] HeapData sub(const StackValue &a, const StackValue &b);
[[nodiscard]] HeapData mul(const StackValue &a, const StackValue &b);
[[nodiscard]] HeapData div(const StackValue &a, const StackValue &b);
[[nodiscard]] HeapData mod(const StackValue &a, const StackValue &b);
[[nodiscard]] HeapData pow(const StackValue &a, const StackValue &b);

// Comparison
[[nodiscard]] std::partial_ordering
compare(const StackValue &a, const StackValue &b);

// Unary ops
[[nodiscard]] HeapData negative(const StackValue &sv);
[[nodiscard]] HeapData not_op(const StackValue &sv);

} // namespace l3::runtime
