module l3.runtime;

import utils;
import :error;
import :primitive;
import :gc_value;

namespace l3::runtime {

namespace {

template <typename Container>
void multiply_container_inplace(
    Container &container, const Primitive &primitive
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

  const auto span = std::span(container);
  for (std::size_t i = 0; i < count - 1; ++i) {
    container.insert(container.end(), span.begin(), span.end());
  }
};

template <typename Container>
auto multiply_container(
    const Container &container, const Primitive &primitive
) {
  auto result = container;
  multiply_container_inplace(result, primitive);
  return result;
}

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

std::pair<std::int64_t, std::int64_t>
slice_bounds(std::size_t size, Slice slice) {
  const auto [start_opt, end_opt] = slice;
  auto start = start_opt.value_or(0);
  auto end = end_opt.value_or(static_cast<std::int64_t>(size));

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

  return {start, end};
}

template <typename T> utils::optional_cref<T> as_impl(const Value &v) {
  return v.visit(
      [](const T &val) -> utils::optional_cref<T> { return val; },
      [](const auto &) -> utils::optional_cref<T> { return std::nullopt; }
  );
}

template <typename T> utils::optional_ref<T> as_mut_impl(Value &v) {
  return v.visit(
      [](T &val) -> utils::optional_ref<T> { return val; },
      [](auto &) -> utils::optional_ref<T> { return std::nullopt; }
  );
}

template <typename T> bool is_impl(const Value &v) {
  return v.visit(
      [](const T &) { return true; }, [](const auto &) { return false; }
  );
}

template <typename... Vis>
decltype(auto) visit_flat(const StackValue &sv, Vis &&...vis) {
  auto overloaded = match::Overloaded{std::forward<Vis>(vis)...};
  return sv.visit(overloaded, [&](GCValue *gcv) {
    return gcv->get_value().visit(overloaded);
  });
}

template <typename... Vis>
decltype(auto) visit_flat(const Value &v, Vis &&...vis) {
  return match::match(v.get_inner(), std::forward<Vis>(vis)...);
}

template <typename... Handlers>
auto visit_pair(const auto &a, const auto &b, Handlers &&...handlers) {
  auto visitor = match::Overloaded{std::forward<Handlers>(handlers)...};
  return visit_flat(a, [&](const auto &flat_a) {
    return visit_flat(b, [&](const auto &flat_b) {
      return visitor(flat_a, flat_b);
    });
  });
}

} // namespace

Value::Value() : inner{Nil{}} {}
Value::Value(Nil /*unused*/) : inner{Nil{}} {}
Value::Value(Primitive primitive) : inner{primitive} {}
Value::Value(const Value &other)
    : inner{other.visit(
          [](const function_type &f) -> variant {
            return std::make_unique<Function>(*f);
          },
          [](const auto &value) -> variant { return value; }
      )} {}

Value &Value::operator=(const Value &other) {
  other.visit(
      [this](const function_type &f) {
        inner = std::make_unique<Function>(*f);
      },
      [this](const auto &value) { inner = value; }
  );
  return *this;
}

Value::Value(std::unique_ptr<Function> &&function)
    : inner{std::move(function)} {}
Value::Value(const Function &function)
    : inner{std::make_unique<Function>(function)} {}
Value::Value(Function &&function)
    : inner{std::make_unique<Function>(std::move(function))} {}
Value::Value(vector_type &&vector) : inner{std::move(vector)} {}
Value::Value(string_type &&string) : inner{std::move(string)} {}

Value Value::add(const Value &other) const {
  return match::match(
      std::forward_as_tuple(inner, other.inner),
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs + rhs};
      },
      [](const vector_type &lv, const vector_type &rv) -> Value {
        auto result = lv;
        result.append_range(rv);
        return {std::move(result)};
      },
      [](const string_type &ls, const string_type &rs) -> Value {
        return {ls + rs};
      },
      [&other, this](const auto &, const auto &) -> Value {
        throw UnsupportedOperation("addition", type_name(), other.type_name());
      }
  );
}

void Value::add_assign(const Value &other) {
  match::match(
      std::forward_as_tuple(inner, other.inner),
      [](Primitive &lhs, const Primitive &rhs) { lhs = lhs + rhs; },
      [](vector_type &lv, const vector_type &rv) { lv.append_range(rv); },
      [](string_type &ls, const string_type &rs) { ls.append_range(rs); },
      [&other, this](auto &, const auto &) {
        throw UnsupportedOperation("addition", type_name(), other.type_name());
      }
  );
}

Value Value::sub(const Value &other) const {
  return binary_op(
      "subtraction",
      *this,
      other,
      [](const Primitive &lhs, const Primitive &rhs) {
        return Value{lhs - rhs};
      }
  );
}

Value Value::mul(const Value &other) const {
  return match::match(
      std::forward_as_tuple(inner, other.inner),
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs * rhs};
      },
      [](const Primitive &count_prim, const vector_type &vec) -> Value {
        return multiply_container(vec, count_prim);
      },
      [](const Primitive &count_prim, const string_type &str) -> Value {
        return multiply_container(str, count_prim);
      },
      [](const vector_type &vec, const Primitive &count_prim) -> Value {
        return multiply_container(vec, count_prim);
      },
      [](const string_type &str, const Primitive &count_prim) -> Value {
        return multiply_container(str, count_prim);
      },
      [&other, this](const auto &, const auto &) -> Value {
        throw UnsupportedOperation(
            "multiplication", type_name(), other.type_name()
        );
      }
  );
}

Value Value::div(const Value &other) const {
  return binary_op(
      "division", *this, other, [](const Primitive &lhs, const Primitive &rhs) {
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
  return match::match(
      std::forward_as_tuple(inner, other.inner),
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
      [](const Nil &) { return false; },
      [](const function_type &) -> bool {
        throw TypeError(
            "cannot convert a function to bool, did you mean to call the "
            "function?"
        );
      },
      [](const vector_type &value) { return !value.empty(); },
      [](const string_type &value) { return !value.empty(); }
  );
}

bool Value::is_nil() const { return is_impl<Nil>(*this); }
bool Value::is_function() const { return is_impl<function_type>(*this); }
bool Value::is_primitive() const { return is_impl<Primitive>(*this); }
bool Value::is_vector() const { return is_impl<vector_type>(*this); }
bool Value::is_string() const { return is_impl<string_type>(*this); }

utils::optional_cref<Primitive> Value::as_primitive() const {
  return as_impl<Primitive>(*this);
}

utils::optional_cref<Value::vector_type> Value::as_vector() const {
  return as_impl<vector_type>(*this);
}

utils::optional_ref<Value::vector_type> Value::as_mut_vector() {
  return as_mut_impl<vector_type>(*this);
}

utils::optional_cref<Value::string_type> Value::as_string() const {
  return as_impl<string_type>(*this);
}

Value Value::slice(Slice slice) const {
  return visit(
      [slice](const vector_type &vector) -> Value {
        const auto [start, end] = slice_bounds(vector.size(), slice);
        return {
            Value::vector_type(vector.begin() + start, vector.begin() + end)
        };
      },
      [slice](const string_type &string) -> Value {
        const auto [start, end] = slice_bounds(string.size(), slice);
        return {string.substr(
            static_cast<Value::string_type::size_type>(start),
            static_cast<Value::string_type::size_type>(end - start)
        )};
      },
      [this](const auto &) -> Value {
        throw TypeError("cannot slice a {} value", type_name());
      }
  );
}

Value Value::not_op() const {
  return visit(
      [](const Primitive &primitive) -> Value { return {!primitive}; },
      [this](const auto &) -> Value { return {Primitive{!is_truthy()}}; }
  );
}

Value Value::negative() const {
  return visit(
      [](const Primitive &primitive) -> Value { return {-primitive}; },
      [this](const auto &) -> Value {
        throw UnsupportedOperation("cannot negate a {} value", type_name());
      }
  );
}

std::string_view Value::type_name() const {
  using std::string_view_literals::operator""sv;
  return visit(
      [](const Primitive &primitive) { return primitive.type_name(); },
      [](const Nil &) { return "nil"sv; },
      [](const function_type &) { return "function"sv; },
      [](const Value::vector_type &) { return "vector"sv; },
      [](const Value::string_type &) { return "string"sv; }
  );
}

// ---------------------------------------------------------------------------
// StackValue operations — using unified visitor
// ---------------------------------------------------------------------------

Value to_value(const StackValue &sv) {
  return sv.visit(
      [](Nil) -> Value { return {}; },
      [](const Primitive &p) -> Value { return Value{p}; },
      [](GCValue *gcv) -> Value {
        return gcv->get_value().visit(
            [](const std::unique_ptr<Function> &f) -> Value {
              return Value{*f};
            },
            [](const std::vector<StackValue> &v) -> Value {
              return Value{std::vector<StackValue>(v)};
            },
            [](const std::string &s) -> Value { return Value{std::string(s)}; },
            [](Primitive p) -> Value { return Value{p}; },
            [](Nil) -> Value { return {}; }
        );
      }
  );
}

Value add(const StackValue &a, const StackValue &b) {
  return visit_pair(
      a,
      b,
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs + rhs};
      },
      [](const std::string &ls, const std::string &rs) -> Value {
        return {ls + rs};
      },
      [](const std::vector<StackValue> &lv,
         const std::vector<StackValue> &rv) -> Value {
        std::vector<StackValue> result;
        result.reserve(lv.size() + rv.size());
        result.append_range(lv);
        result.append_range(rv);
        return {std::move(result)};
      },
      [&](const auto &, const auto &) -> Value {
        throw UnsupportedOperation("addition", a.type_name(), b.type_name());
      }
  );
}

Value sub(const StackValue &a, const StackValue &b) {
  return visit_pair(
      a,
      b,
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs - rhs};
      },
      [](const auto &, const auto &) -> Value {
        throw UnsupportedOperation("subtraction requires primitives");
      }
  );
}

Value mul(const StackValue &a, const StackValue &b) {
  return visit_pair(
      a,
      b,
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs * rhs};
      },
      [](const Primitive &count, const std::vector<StackValue> &vec) -> Value {
        return multiply_container(vec, count);
      },
      [](const Primitive &count, const std::string &str) -> Value {
        return multiply_container(str, count);
      },
      [](const std::vector<StackValue> &vec, const Primitive &count) -> Value {
        return multiply_container(vec, count);
      },
      [](const std::string &str, const Primitive &count) -> Value {
        return multiply_container(str, count);
      },
      [](const auto &, const auto &) -> Value {
        throw UnsupportedOperation("multiplication");
      }
  );
}

Value div(const StackValue &a, const StackValue &b) {
  return visit_pair(
      a,
      b,
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs / rhs};
      },
      [](const auto &, const auto &) -> Value {
        throw UnsupportedOperation("division requires primitives");
      }
  );
}

Value mod(const StackValue &a, const StackValue &b) {
  return visit_pair(
      a,
      b,
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs % rhs};
      },
      [](const auto &, const auto &) -> Value {
        throw UnsupportedOperation("modulo requires primitives");
      }
  );
}

std::partial_ordering compare(const StackValue &a, const StackValue &b) {
  return visit_pair(
      a,
      b,
      []<typename T>(const T &lhs, const T &rhs) -> std::partial_ordering
        requires requires(T lhs, T rhs) { lhs <=> rhs; }
      { return lhs <=> rhs; },
      [](const std::vector<StackValue> &lv,
         const std::vector<StackValue> &rv) -> std::partial_ordering {
        if (lv.size() != rv.size()) {
          return lv.size() <=> rv.size();
        }
        for (std::size_t i = 0; i < lv.size(); ++i) {
          const auto elem_cmp = compare(lv[i], rv[i]);
          if (elem_cmp != std::partial_ordering::equivalent) {
            return elem_cmp;
          }
        }
        return std::partial_ordering::equivalent;
      },
      [](const auto &, const auto &) {
        return std::partial_ordering::unordered;
      }
      );
}

Value negative(const StackValue &sv) {
  return visit_flat(
      sv,
      [](const Primitive &p) -> Value { return {-p}; },
      [](const auto &) -> Value {
        throw UnsupportedOperation("cannot negate a non-numeric value");
      }
  );
}

Value not_op(const StackValue &sv) {
  return visit_flat(
      sv,
      [](const Primitive &p) -> Value { return {!p}; },
      [](Nil) -> Value { return {Primitive{true}}; },
      [](const std::string &s) -> Value { return {Primitive{s.empty()}}; },
      [](const std::vector<StackValue> &v) -> Value {
        return {Primitive{v.empty()}};
      },
      [](const auto &) -> Value {
        throw TypeError("cannot convert a function to bool");
      }
  );
}

NewValue index(const StackValue &container, const StackValue &index_sv) {
  const auto index_opt =
      index_sv.as_primitive().and_then(&Primitive::as_integer);
  if (!index_opt) {
    throw TypeError("index to a container must be an integer");
  }
  if (*index_opt < 0) {
    throw ValueError("index out of bounds");
  }
  const auto idx = static_cast<std::size_t>(*index_opt);

  return visit_flat(
      container,
      [&](const std::vector<StackValue> &vec) -> NewValue {
        if (idx >= vec.size()) {
          throw ValueError("index out of bounds");
        }
        return vec[idx];
      },
      [&](const std::string &s) -> NewValue {
        if (idx >= s.size()) {
          throw ValueError("index out of bounds");
        }
        return Value{std::string{s.substr(idx, 1)}};
      },
      [&](const auto &) -> NewValue {
        throw TypeError("cannot index a {} value", container.type_name());
      }
  );
}

StackValue &index_mut(StackValue &container, const StackValue &index_sv) {
  const auto index_opt =
      index_sv.as_primitive().and_then(&Primitive::as_integer);
  if (!index_opt) {
    throw TypeError("index must be an integer");
  }
  if (*index_opt < 0) {
    throw ValueError("index out of bounds");
  }
  const auto idx = static_cast<std::size_t>(*index_opt);

  auto *gcv = container.get_gc_ptr();
  if (gcv == nullptr) {
    throw TypeError("cannot index a {} value", container.type_name());
  }
  return gcv->get_value_mut().visit(
      [&](std::vector<StackValue> &vec) -> StackValue & {
        if (idx >= vec.size()) {
          throw ValueError("index out of bounds");
        }
        return vec[idx];
      },
      [&](const auto &) -> StackValue & {
        throw TypeError("cannot index a {} value", container.type_name());
      }
  );
}

} // namespace l3::runtime
