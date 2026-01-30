#include "vm/value.hpp"
#include "vm/error.hpp"
#include "vm/function.hpp"
#include <memory>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

namespace l3::vm {

namespace {

template <typename Result, typename... Handlers>
auto handle_bin_op(
    std::string_view name,
    const Primitive &lhs,
    const Primitive &rhs,
    Handlers... handlers
) {
  return match::match(
      std::forward_as_tuple(lhs.get_inner(), rhs.get_inner()),
      handlers...,
      [name](const auto &lhs, const auto &rhs) -> Result {
        throw UnsupportedOperation(name, lhs, rhs);
      }
  );
}

template <typename Result, typename... Handlers>
auto handle_un_op(
    std::string_view name, const Primitive &value, Handlers... handlers
) {
  return value.visit(handlers..., [name](const auto &value) -> Result {
    throw UnsupportedOperation(name, value);
  });
}

} // namespace

Primitive operator+(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op<Primitive>(
      "addition",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires(!std::is_same_v<T, bool>)
      { return Primitive{lhs + rhs}; }
      );
}

Primitive operator-(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op<Primitive>(
      "subtraction",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs - rhs; } &&
                 (!std::is_same_v<T, bool>)
      { return Primitive{lhs - rhs}; }
      );
}

Primitive operator*(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op<Primitive>(
      "multiplication",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs * rhs; } &&
                     (!std::is_same_v<T, bool>)
      { return Primitive{lhs * rhs}; },
      [](const std::int64_t &lhs, const std::string &rhs) -> Primitive {
        if (lhs < 0) {
          throw UnsupportedOperation(
              "negative string multiplication not supported"
          );
        }
        std::string result;
        result.reserve(static_cast<std::size_t>(lhs) * rhs.size());
        for (std::size_t i = 0; i < static_cast<std::size_t>(lhs); ++i) {
          result += rhs;
        }
        return Primitive{result};
      }
      );
}

Primitive operator/(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op<Primitive>(
      "division",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs / rhs; } &&
                 (!std::is_same_v<T, bool>)
      {
        if (rhs == static_cast<T>(0)) {
          throw UnsupportedOperation("division by zero");
        }
        return Primitive{lhs / rhs};
      }
      );
}

Primitive operator%(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op<Primitive>(
      "modulo",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs % rhs; } &&
                 (!std::is_same_v<T, bool>)
      { return Primitive{lhs % rhs}; }
      );
}

Primitive operator&&(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{lhs.is_truthy() && rhs.is_truthy()};
}

Primitive operator||(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{lhs.is_truthy() || rhs.is_truthy()};
}

Primitive operator!(const Primitive &value) {
  return Primitive{!value.is_truthy()};
}

Primitive operator-(const Primitive &value) {
  return handle_un_op<Primitive>(
      "minus",
      value,
      []<typename T>(const T &value) -> Primitive
        requires requires(T value) { -value; } && (!std::is_same_v<T, bool>)
      { return Primitive{-value}; }
      );
}

Primitive operator+(const Primitive &value) {
  return handle_un_op<Primitive>(
      "plus",
      value,
      []<typename T>(const T &value) -> Primitive
        requires requires(T value) { +value; } && (!std::is_same_v<T, bool>)
      { return Primitive{+value}; }
      );
}

namespace {
int operator<=>(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op<int>(
      "comparison",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> int
        requires(!std::is_same_v<T, bool>)
      {
        auto result = lhs <=> rhs;
        if (result < 0) {
          return -1;
        }
        if (result > 0) {
          return 1;
        }
        return 0;
      }
      );
}
} // namespace

Primitive operator!=(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) != 0};
}

Primitive operator==(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) == 0};
}

Primitive operator<(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) < 0};
}

Primitive operator>(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) > 0};
}

Primitive operator<=(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) <= 0};
}

Primitive operator>=(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) >= 0};
}

Value Value::add(const Value &other) const {
  return match::match(
      std::forward_as_tuple(inner, other.inner),
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return Value{lhs + rhs};
      },
      [](const vector_type &lhs, const vector_type &rhs) -> Value {
        auto result = lhs;
        result.insert(result.end(), rhs.begin(), rhs.end());
        return Value{std::move(result)};
      },
      [&other, this](const auto &, const auto &) -> Value {
        throw UnsupportedOperation(
            "add between {} and {} not supported",
            type_name(),
            other.type_name()
        );
      }
  );
}

Value Value::sub(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs - rhs; },
      "subtract"
  );
}

Value Value::mul(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs * rhs; },
      "multiply"
  );
}

Value Value::div(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs / rhs; },
      "divide"
  );
}

Value Value::mod(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs % rhs; },
      "modulo"
  );
}

Value Value::and_op(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs && rhs; },
      "perform logical and on"
  );
}

Value Value::or_op(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs || rhs; },
      "perform logical or on"
  );
}

Value Value::equal(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs == rhs; },
      "compare"
  );
};

Value Value::not_equal(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs != rhs; },
      "compare"
  );
}

Value Value::greater(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs > rhs; },
      "compare"
  );
}

Value Value::greater_equal(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs >= rhs; },
      "compare"
  );
}

Value Value::less(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs < rhs; },
      "compare"
  );
}

Value Value::less_equal(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs <= rhs; },
      "compare"
  );
}

bool Primitive::is_truthy() const {
  return visit(
      [](const bool &value) { return value; },
      [](const std::int64_t &value) { return value != 0; },
      [](const string_type &value) { return !value.empty(); },
      [](const double & /*value*/) -> bool {
        throw RuntimeError("cannot convert a floating point number to bool");
      }
  );
}
bool Value::is_truthy() const {
  return visit(
      [](const Primitive &primitive) { return primitive.is_truthy(); },
      [](const Nil & /*value*/) { return false; },
      [](const function_type & /*value*/) -> bool {
        throw TypeError(
            "cannot convert a function to bool, did you mean to call the "
            "function?"
        );
      },
      [](const Value::vector_type &value) { return !value.empty(); }
  );
}

std::optional<std::reference_wrapper<const Primitive::string_type>>
Primitive::as_string() const {
  return visit(
      [](const string_type &value) -> utils::optional_cref<string_type> {
        return std::cref(value);
      },
      [](const auto &) -> utils::optional_cref<string_type> {
        return std::nullopt;
      }
  );
}
std::optional<double> Primitive::as_double() const {
  return visit(
      [](const double &value) -> std::optional<double> { return value; },
      [](const auto &) -> std::optional<double> { return std::nullopt; }
  );
}
std::optional<std::int64_t> Primitive::as_integer() const {
  return visit(
      [](const std::int64_t &value) -> std::optional<std::int64_t> {
        return value;
      },
      [](const auto &) -> std::optional<std::int64_t> { return std::nullopt; }
  );
}
std::optional<bool> Primitive::as_bool() const {
  return visit(
      [](const bool &value) -> std::optional<bool> { return value; },
      [](const auto &) -> std::optional<bool> { return std::nullopt; }
  );
}

Primitive::Primitive(bool value) : inner{value} {}
Primitive::Primitive(std::int64_t value) : inner{value} {}
Primitive::Primitive(double value) : inner{value} {}
Primitive::Primitive(const std::string &value) : inner{value} {}
Primitive::Primitive(std::string &&value) : inner{std::move(value)} {}
Value::Value() : inner{Nil{}} {}
Value::Value(Nil /*unused*/) : inner{Nil{}} {}
Value::Value(Primitive &&primitive) : inner{std::move(primitive)} {}
Value::Value(function_type function) : inner{std::move(function)} {}
Value::Value(vector_type &&vector) : inner{std::move(vector)} {}
Value::Value(Function &&function)
    : inner{std::make_shared<Function>(std::move(function))} {}

bool Value::is_nil() const { return std::holds_alternative<Nil>(inner); }
bool Value::is_function() const {
  return std::holds_alternative<function_type>(inner);
}
bool Value::is_primitive() const {
  return std::holds_alternative<Primitive>(inner);
}

Value Value::binary_op(
    const Value &other,
    const std::function<Value(const Primitive &, const Primitive &)> &op_fn,
    const std::string &op_name
) const {
  return match::match(
      std::forward_as_tuple(inner, other.inner),
      [&op_fn](const Primitive &lhs, const Primitive &rhs) -> Value {
        return op_fn(lhs, rhs);
      },
      [&op_name, &other, this](const auto &, const auto &) -> Value {
        throw UnsupportedOperation(
            "{} between {} and {} not supported",
            op_name,
            type_name(),
            other.type_name()
        );
      }
  );
}

namespace {

size_t value_to_index(const Value &value) {
  const auto index_opt = value.as_primitive().and_then(&Primitive::as_integer);

  if (!index_opt) {
    throw TypeError("index to a vector must be an integer");
  }

  const auto index = *index_opt;

  if (index < 0LL) {
    throw ValueError("index out of bounds");
  }

  return static_cast<std::size_t>(index);
}

} // namespace

NewValue Value::index(const Value &index_value) const {
  return index(value_to_index(index_value));
}
NewValue Value::index(size_t index) const {
  return visit(
      [&index](const std::vector<RefValue> &values) -> NewValue {
        if (index >= values.size()) {
          throw ValueError("index out of bounds");
        }
        return values[index];
      },
      [&index](const Primitive &primitive) -> NewValue {
        const auto string_opt = primitive.as_string();

        if (!string_opt) {
          throw TypeError("cannot index a primitive non-string value");
        }

        const auto &string = string_opt->get();

        if (index >= string.size()) {
          throw ValueError("index out of bounds");
        }

        return Primitive{string.substr(index, 1)};
      },
      [this](const auto & /*value*/) -> NewValue {
        throw TypeError("cannot index a {} value", type_name());
      }
  );
}

RefValue &Value::index_mut(const Value &index) {
  return index_mut(value_to_index(index));
}
RefValue &Value::index_mut(size_t index) {
  return visit(
      [&index](std::vector<RefValue> &values) -> RefValue & {
        if (index >= values.size()) {
          throw ValueError("index out of bounds");
        }
        return values[index];
      },
      [this](const auto & /*value*/) -> RefValue & {
        throw TypeError("cannot mutaly index a {} value", type_name());
      }
  );
}

using copt_primitive_type = utils::optional_cref<Primitive>;

copt_primitive_type Value::as_primitive() const {
  return visit(
      [](const Primitive &primitive) -> copt_primitive_type {
        return primitive;
      },
      [](const auto &) -> copt_primitive_type { return std::nullopt; }
  );
}

using copt_function_type = utils::optional_cref<Value::function_type>;

copt_function_type Value::as_function() const {
  return visit(
      [](const function_type &function) -> copt_function_type {
        return function;
      },
      [](const auto &) -> copt_function_type { return std::nullopt; }
  );
}
using copt_vector_type = utils::optional_cref<Value::vector_type>;
copt_vector_type Value::as_vector() const {
  return visit(
      [](const Value::vector_type &vector) -> copt_vector_type {
        return vector;
      },
      [](const auto &) -> copt_vector_type { return std::nullopt; }
  );
}
using opt_vector_type = utils::optional_ref<Value::vector_type>;
opt_vector_type Value::as_mut_vector() {
  return visit(
      [](Value::vector_type &vector) -> opt_vector_type { return vector; },
      [](auto &) -> opt_vector_type { return std::nullopt; }
  );
}

namespace {

Value slice_vector(const Value::vector_type &vector, Slice slice) {
  const auto [start_opt, end_opt] = slice;
  auto start = start_opt.value_or(0);
  auto end = end_opt.value_or(vector.size());

  if (start > end) {
    throw ValueError("start index must be less than end index");
  }

  if (start < 0) {
    start += static_cast<std::int64_t>(vector.size());
  }
  if (end < 0) {
    end += static_cast<std::int64_t>(vector.size());
  }

  if (static_cast<std::size_t>(end) > vector.size()) {
    throw ValueError("end index out of bounds");
  }
  if (static_cast<std::size_t>(start) > vector.size()) {
    throw ValueError("start index out of bounds");
  }
  return Value{
      Value::vector_type(vector.begin() + start, vector.begin() + end)
  };
}
Value slice_string(Slice slice, const std::string &string) {
  const auto [start_opt, end_opt] = slice;
  auto start = start_opt.value_or(0);
  auto end = end_opt.value_or(string.size());

  if (start > end) {
    throw ValueError("start index must be less than end index");
  }

  if (start < 0) {
    start += static_cast<std::int64_t>(string.size());
  }
  if (end < 0) {
    end += static_cast<std::int64_t>(string.size());
  }

  if (static_cast<std::size_t>(end) > string.size()) {
    throw ValueError("end index out of bounds");
  }
  if (static_cast<std::size_t>(start) > string.size()) {
    throw ValueError("start index out of bounds");
  }
  return Primitive{string.substr(
      static_cast<std::size_t>(start), static_cast<std::size_t>(end - start)
  )};
}

} // namespace

Value Value::slice(Slice slice) const {
  return visit(
      [slice](const vector_type &vector) -> Value {
        return slice_vector(vector, slice);
      },
      [this, slice](const Primitive &primitive) -> Value {
        if (const auto string_opt = primitive.as_string()) {
          return slice_string(slice, *string_opt);
        }

        throw TypeError("cannot slice a {} value", type_name());
      },
      [this](const auto & /*value*/) -> Value {
        throw TypeError("cannot slice a {} value", type_name());
      }
  );
}

Value Value::not_op() const {
  return visit(
      [](const Primitive &primitive) -> Value { return Value{!primitive}; },
      [this](const auto & /*value*/) -> Value {
        return Value{Primitive{!is_truthy()}};
      }
  );
}

// Value Value::positive() const {}

Value Value::negative() const {
  return visit(
      [](const Primitive &primitive) -> Value { return Value{-primitive}; },
      [this](const auto & /*value*/) -> Value {
        throw UnsupportedOperation("cannot negate a {} value", type_name());
      }
  );
}

[[nodiscard]] std::string_view Primitive::type_name() const {
  using std::string_view_literals::operator""sv;
  return visit(
      [](const bool & /*value*/) { return "bool"sv; },
      [](const std::int64_t & /*value*/) { return "integer"sv; },
      [](const double & /*value*/) { return "double"sv; },
      [](const std::string & /*value*/) { return "string"sv; }
  );
}

[[nodiscard]] std::string_view Value::type_name() const {
  using std::string_view_literals::operator""sv;
  return visit(
      [](const Primitive &primitive) { return primitive.type_name(); },
      [](const Nil & /*value*/) { return "nil"sv; },
      [](const function_type & /*value*/) { return "function"sv; },
      [](const Value::vector_type & /*value*/) { return "vector"sv; }
  );
}

} // namespace l3::vm
