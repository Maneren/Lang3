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

namespace l3::vm {

Value Value::add(const Value &other) const {
  return match::match(
      std::forward_as_tuple(inner, other.inner),
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return Value{lhs + rhs};
      },
      [](const vector_ptr_type &lhs, const vector_ptr_type &rhs) -> Value {
        auto result = *lhs;
        result.insert(result.end(), rhs->begin(), rhs->end());
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
      [](const Value::vector_ptr_type &value) { return !value->empty(); }
  );
}

Value::Value() : inner{Nil{}} {}
Value::Value(Nil /*unused*/) : inner{Nil{}} {}
Value::Value(Primitive &&primitive) : inner{std::move(primitive)} {}
Value::Value(function_type &&function) : inner{std::move(function)} {}
Value::Value(vector_ptr_type &&vector) : inner{std::move(vector)} {}
Value::Value(vector_type &&vector)
    : inner{std::make_shared<vector_type>(std::move(vector))} {}
Value::Value(Function &&function)
    : inner{std::make_shared<Function>(std::move(function))} {}

Value Value::clone() const {
  return visit([](auto inner) { return Value{std::move(inner)}; });
}

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
      [&index](const vector_ptr_type &values) -> NewValue {
        if (index >= values->size()) {
          throw ValueError("index out of bounds");
        }
        return (*values)[index];
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
      [&index](const vector_ptr_type &values) -> RefValue & {
        if (index >= values->size()) {
          throw ValueError("index out of bounds");
        }
        return (*values)[index];
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
      [](const Value::vector_ptr_type &vector) -> copt_vector_type {
        return *vector;
      },
      [](const auto &) -> copt_vector_type { return std::nullopt; }
  );
}
using opt_vector_type = utils::optional_ref<Value::vector_type>;
opt_vector_type Value::as_mut_vector() {
  return visit(
      [](Value::vector_ptr_type &vector) -> opt_vector_type { return *vector; },
      [](auto &) -> opt_vector_type { return std::nullopt; }
  );
}

namespace {

Value slice_vector(const Value::vector_ptr_type &vector, Slice slice) {
  const auto [start_opt, end_opt] = slice;
  auto start = start_opt.value_or(0);
  auto end = end_opt.value_or(vector->size());

  if (start > end) {
    throw ValueError("start index must be less than end index");
  }

  if (start < 0) {
    start += static_cast<std::int64_t>(vector->size());
  }
  if (end < 0) {
    end += static_cast<std::int64_t>(vector->size());
  }

  if (static_cast<std::size_t>(end) > vector->size()) {
    throw ValueError("end index out of bounds");
  }
  if (static_cast<std::size_t>(start) > vector->size()) {
    throw ValueError("start index out of bounds");
  }
  return {Value::vector_type(vector->begin() + start, vector->begin() + end)};
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
      [slice](const vector_ptr_type &vector) -> Value {
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

std::string_view Value::type_name() const {
  using std::string_view_literals::operator""sv;
  return visit(
      [](const Primitive &primitive) { return primitive.type_name(); },
      [](const Nil & /*value*/) { return "nil"sv; },
      [](const function_type & /*value*/) { return "function"sv; },
      [](const Value::vector_ptr_type & /*value*/) { return "vector"sv; }
  );
}

} // namespace l3::vm

// Formatters for Value types
namespace l3::vm {} // namespace l3::vm
