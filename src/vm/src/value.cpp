module;

#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <variant>

module l3.vm;

import utils;
import :error;
import :primitive;
import :ref_value;
import :variable;

namespace l3::vm {

namespace {

template <bool backup = true>
auto binary_op(
    std::string_view name,
    const Value &lhs,
    const Value &rhs,
    const auto &...handlers
) {
  using R = std::invoke_result_t<
      match::Overloaded<std::decay_t<decltype(handlers)>...>,
      const Primitive &,
      const Primitive &>;
  return match::match(
      std::forward_as_tuple(lhs.get_inner(), rhs.get_inner()),
      handlers...,
      [name, &lhs, &rhs](const auto &, const auto &) -> R
        requires(backup)
      {
        throw UnsupportedOperation(
            "{} between {} and {} not supported",
            name,
            lhs.type_name(),
            rhs.type_name()
        );
      }
      );
}

} // namespace

Value Value::add(const Value &other) const {
  return match::match(
      std::forward_as_tuple(inner, other.inner),
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs + rhs};
      },
      []<ValueContainer T>(const T &lhs, const T &rhs) -> Value {
        auto result = lhs;
        result.insert(result.end(), rhs.begin(), rhs.end());
        return {std::move(result)};
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

void Value::add_assign(const Value &other) {
  match::match(
      std::forward_as_tuple(inner, other.inner),
      [](Primitive &lhs, const Primitive &rhs) { lhs = lhs + rhs; },
      []<ValueContainer T>(T &lhs, const T &rhs) {
        lhs.insert(lhs.end(), rhs.begin(), rhs.end());
      },
      [&other, this](auto &, const auto &) {
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
      "subtract", *this, other, [](const Primitive &lhs, const Primitive &rhs) {
        return Value{lhs - rhs};
      }
  );
}

namespace {

void multiply_container_inplace(
    ValueContainer auto &container, const Primitive &primitive
) {
  const auto count_opt = primitive.as_integer();
  if (!count_opt) {
    throw UnsupportedOperation("container multiplication requires an integer");
  }
  if (*count_opt <= 0) {
    throw UnsupportedOperation(
        "container can be multiplied only by a positive integer"
    );
  }
  const auto count = static_cast<std::size_t>(*count_opt);

  container.reserve(count * container.size());

  // Since we reserve the space, the container won't reallocate and as such
  // iterators won't be invalidated
  const auto span = std::span(container);
  for (std::size_t i = 0; i < count - 1; ++i) {
    container.insert(container.end(), span.begin(), span.end());
  }
};

auto multiply_container(
    const ValueContainer auto &container, const Primitive &primitive
) {
  auto result = container;
  multiply_container_inplace(result, primitive);
  return result;
}

} // namespace

Value Value::mul(const Value &other) const {
  return match::match(
      std::forward_as_tuple(inner, other.inner),
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs * rhs};
      },
      [](const Primitive &count_prim, const ValueContainer auto &vec) -> Value {
        return multiply_container(vec, count_prim);
      },
      [](const ValueContainer auto &vec, const Primitive &count_prim) -> Value {
        return multiply_container(vec, count_prim);
      },
      [&other, this](const auto &, const auto &) -> Value {
        throw UnsupportedOperation(
            "multiply between {} and {} not supported",
            type_name(),
            other.type_name()
        );
      }
  );
}

void Value::mul_assign(const Value &other) {
  match::match(
      std::forward_as_tuple(inner, other.inner),
      [](Primitive &lhs, const Primitive &rhs) { lhs = lhs * rhs; },
      [](ValueContainer auto &lhs, const Primitive &rhs) {
        multiply_container_inplace(lhs, rhs);
      },
      [&other, this](auto &, const auto &) {
        throw UnsupportedOperation(
            "multiplication between {} and {} not supported",
            type_name(),
            other.type_name()
        );
      }
  );
}

Value Value::div(const Value &other) const {
  return binary_op(
      "divide", *this, other, [](const Primitive &lhs, const Primitive &rhs) {
        return Value{lhs / rhs};
      }
  );
}

Value Value::mod(const Value &other) const {
  return binary_op(
      "modulo", *this, other, [](const Primitive &lhs, const Primitive &rhs) {
        return Value{lhs % rhs};
      }
  );
}

std::partial_ordering Value::compare(const Value &other) const {
  return binary_op<false>(
      "compare",
      *this,
      other,
      []<typename T>(const T &lhs, const T &rhs) -> std::partial_ordering
        requires requires(T lhs, T rhs) { lhs <=> rhs; }
      { return lhs <=> rhs; },
      [](const auto &, const auto &) -> std::partial_ordering {
        return std::partial_ordering::unordered;
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
      [](const ValueContainer auto &value) { return !value.empty(); }
  );
}

Value::Value() : inner{Nil{}} {}
Value::Value(Nil /*unused*/) : inner{Nil{}} {}
Value::Value(Primitive &&primitive) : inner{std::move(primitive)} {}
Value::Value(function_type &&function) : inner{std::move(function)} {}
Value::Value(vector_type &&vector) : inner{std::move(vector)} {}
Value::Value(string_type &&string) : inner{std::move(string)} {}

bool Value::is_nil() const { return std::holds_alternative<Nil>(inner); }
bool Value::is_function() const {
  return std::holds_alternative<function_type>(inner);
}
bool Value::is_primitive() const {
  return std::holds_alternative<Primitive>(inner);
}
bool Value::is_string() const {
  return std::holds_alternative<string_type>(inner);
}

namespace {

size_t value_to_index(const Value &value) {
  const auto index_opt = value.as_primitive().and_then(&Primitive::as_integer);

  if (!index_opt) {
    throw TypeError("index to a container must be an integer");
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
      [&index](const vector_type &values) -> NewValue {
        if (index >= values.size()) {
          throw ValueError("index out of bounds");
        }
        return values[index];
      },
      [&index](const string_type &string) -> NewValue {
        if (index >= string.size()) {
          throw ValueError("index out of bounds");
        }

        return {string.substr(index, 1)};
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
      [&index](vector_type &values) -> RefValue & {
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

using copt_function_type = utils::optional_cref<Function>;

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

using copt_string_type = utils::optional_cref<Value::string_type>;
copt_string_type Value::as_string() const {
  return visit(
      [](const string_type &value) -> copt_string_type {
        return std::cref(value);
      },
      [](const auto &) -> copt_string_type { return std::nullopt; }
  );
}

namespace {

Value slice_vector(const Value::vector_type &vector, Slice slice) {
  const auto [start_opt, end_opt] = slice;
  auto start = start_opt.value_or(0);
  auto size = vector.size();
  auto end = end_opt.value_or(size);

  if (start > end) {
    throw ValueError("start index must be less than end index");
  }

  if (start < 0) {
    start += static_cast<std::int64_t>(size);
  }
  if (end < 0) {
    end += static_cast<std::int64_t>(size);
  }

  if (std::cmp_greater(end, size)) {
    throw ValueError("end index out of bounds");
  }
  if (std::cmp_greater(start, size)) {
    throw ValueError("start index out of bounds");
  }
  return {Value::vector_type(vector.begin() + start, vector.begin() + end)};
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
  return {string.substr(
      static_cast<std::size_t>(start), static_cast<std::size_t>(end - start)
  )};
}

} // namespace

Value Value::slice(Slice slice) const {
  return visit(
      [slice](const vector_type &vector) -> Value {
        return slice_vector(vector, slice);
      },
      [slice](const string_type &string) -> Value {
        return slice_string(slice, string);
      },
      [this](const auto & /*value*/) -> Value {
        throw TypeError("cannot slice a {} value", type_name());
      }
  );
}

Value Value::not_op() const {
  return visit(
      [](const Primitive &primitive) -> Value { return {!primitive}; },
      [this](const auto & /*value*/) -> Value {
        return {Primitive{!is_truthy()}};
      }
  );
}

Value Value::negative() const {
  return visit(
      [](const Primitive &primitive) -> Value { return {-primitive}; },
      [this](const auto & /*value*/) -> Value {
        throw UnsupportedOperation("cannot negate a {} value", type_name());
      }
  );
}

std::string_view Value::type_name() const {
  using std::string_view_literals::operator""sv;
  return visit(
      [](const Primitive &primitive) { return primitive.type_name(); },
      [](const Nil & /*value*/) { return "nil"sv; },
      [](const function_type & /*value*/) { return "function"sv; },
      [](const Value::vector_type & /*value*/) { return "vector"sv; },
      [](const Value::string_type & /*value*/) { return "string"sv; }
  );
}

} // namespace l3::vm
