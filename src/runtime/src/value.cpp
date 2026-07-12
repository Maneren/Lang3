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

  const auto saved = container;
  for (std::size_t i = 0; i < count - 1; ++i) {
    container.append_range(saved);
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
  return sv.visit(overloaded, [&](HeapCell *gcv) {
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

// Operation helpers — work with both Value and StackValue via visit_pair

template <typename T> Value add_op(const T &a, const T &b) {
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
        auto result = lv;
        result.append_range(rv);
        return {std::move(result)};
      },
      [&](const auto &, const auto &) -> Value {
        throw UnsupportedOperation("addition", a.type_name(), b.type_name());
      }
  );
}

template <typename T> Value sub_op(const T &a, const T &b) {
  return visit_pair(
      a,
      b,
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs - rhs};
      },
      [&](const auto &, const auto &) -> Value {
        throw UnsupportedOperation("subtraction", a.type_name(), b.type_name());
      }
  );
}

template <typename T> Value mul_op(const T &a, const T &b) {
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
      [&](const auto &, const auto &) -> Value {
        throw UnsupportedOperation(
            "multiplication", a.type_name(), b.type_name()
        );
      }
  );
}

template <typename T> Value div_op(const T &a, const T &b) {
  return visit_pair(
      a,
      b,
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs / rhs};
      },
      [&](const auto &, const auto &) -> Value {
        throw UnsupportedOperation("division", a.type_name(), b.type_name());
      }
  );
}

template <typename T> Value mod_op(const T &a, const T &b) {
  return visit_pair(
      a,
      b,
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return {lhs % rhs};
      },
      [&](const auto &, const auto &) -> Value {
        throw UnsupportedOperation("modulo", a.type_name(), b.type_name());
      }
  );
}

template <typename T> Value pow_op(const T &a, const T &b) {
  return visit_pair(
      a,
      b,
      [](const Primitive &lhs, const Primitive &rhs) -> Value {
        return Value{match::match(
            std::forward_as_tuple(lhs.get_inner(), rhs.get_inner()),
            [](std::int64_t base, std::int64_t exp) -> Primitive {
              std::int64_t result = 1;
              for (std::int64_t i = 0; i < exp; ++i) {
                result *= base;
              }
              return Primitive{result};
            },
            [](double base, double exp) -> Primitive {
              return Primitive{std::pow(base, exp)};
            },
            [](const auto &, const auto &) -> Primitive {
              throw UnsupportedOperation("power not supported for these types");
            }
        )};
      },
      [&](const auto &, const auto &) -> Value {
        throw UnsupportedOperation("power", a.type_name(), b.type_name());
      }
  );
}

template <typename T> Value negative_op(const T &v) {
  return visit_flat(
      v,
      [](const Primitive &p) -> Value { return {-p}; },
      [&](const auto &) -> Value {
        throw UnsupportedOperation("cannot negate a {} value", v.type_name());
      }
  );
}

template <typename T> std::partial_ordering compare_op(const T &a, const T &b) {
  return visit_pair(
      a,
      b,
      []<typename U>(const U &lhs, const U &rhs) -> std::partial_ordering
        requires requires(U lhs, U rhs) { lhs <=> rhs; }
      { return lhs <=> rhs; },
      [](const std::vector<StackValue> &lv,
         const std::vector<StackValue> &rv) -> std::partial_ordering {
        if (lv.size() != rv.size()) {
          return lv.size() <=> rv.size();
        }
        for (std::size_t i = 0; i < lv.size(); ++i) {
          const auto elem_cmp = compare_op(lv[i], rv[i]);
          if (elem_cmp != std::partial_ordering::equivalent) {
            return elem_cmp;
          }
        }
        return std::partial_ordering::equivalent;
      },
      [](const auto &, const auto &) -> std::partial_ordering {
        return std::partial_ordering::unordered;
      }
      );
}

template <typename T> bool is_truthy_op(const T &v) {
  return visit_flat(
      v,
      [](const Primitive &p) { return p.is_truthy(); },
      [](Nil) { return false; },
      [](const std::string &s) { return !s.empty(); },
      [](const std::vector<StackValue> &vec) { return !vec.empty(); },
      [](const auto &) -> bool {
        throw TypeError(
            "cannot convert a function to bool, did you mean to call the "
            "function?"
        );
      }
  );
}

template <typename T> std::string_view type_name_op(const T &v) {
  using std::string_view_literals::operator""sv;
  return visit_flat(
      v,
      [](const Primitive &p) { return p.type_name(); },
      [](Nil) { return "nil"sv; },
      [](const std::unique_ptr<Function> &) { return "function"sv; },
      [](const std::vector<StackValue> &) { return "vector"sv; },
      [](const std::string &) { return "string"sv; }
  );
}

template <typename T> Value not_op_impl(const T &v) {
  return visit_flat(
      v,
      [](const Primitive &p) -> Value { return {!p}; },
      [](Nil) -> Value { return {Primitive{true}}; },
      [](const std::string &s) -> Value { return {Primitive{s.empty()}}; },
      [](const std::vector<StackValue> &vec) -> Value {
        return {Primitive{vec.empty()}};
      },
      [](const auto &) -> Value {
        throw TypeError(
            "cannot convert a function to bool, did you mean to call the "
            "function?"
        );
      }
  );
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

Value Value::add(const Value &other) const { return add_op(*this, other); }
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
Value Value::sub(const Value &other) const { return sub_op(*this, other); }
Value Value::mul(const Value &other) const { return mul_op(*this, other); }
Value Value::div(const Value &other) const { return div_op(*this, other); }
Value Value::mod(const Value &other) const { return mod_op(*this, other); }
Value Value::pow(const Value &other) const { return pow_op(*this, other); }

std::partial_ordering Value::compare(const Value &other) const {
  return compare_op(*this, other);
}

bool Value::is_truthy() const { return is_truthy_op(*this); }

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

Value Value::not_op() const { return not_op_impl(*this); }

Value Value::negative() const {
  return visit(
      [](const Primitive &primitive) -> Value { return {-primitive}; },
      [this](const auto &) -> Value {
        throw UnsupportedOperation("cannot negate a {} value", type_name());
      }
  );
}

std::string_view Value::type_name() const { return type_name_op(*this); }

bool StackValue::is_truthy() const { return is_truthy_op(*this); }

std::string_view StackValue::type_name() const { return type_name_op(*this); }

// ---------------------------------------------------------------------------
// StackValue operations — using unified visitor
// ---------------------------------------------------------------------------

Value to_value(const StackValue &sv) {
  return sv.visit(
      [](Nil) -> Value { return {}; },
      [](const Primitive &p) -> Value { return Value{p}; },
      [](HeapCell *gcv) -> Value {
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

Value add(const StackValue &a, const StackValue &b) { return add_op(a, b); }
Value sub(const StackValue &a, const StackValue &b) { return sub_op(a, b); }
Value mul(const StackValue &a, const StackValue &b) { return mul_op(a, b); }
Value div(const StackValue &a, const StackValue &b) { return div_op(a, b); }
Value mod(const StackValue &a, const StackValue &b) { return mod_op(a, b); }
Value pow(const StackValue &a, const StackValue &b) { return pow_op(a, b); }
Value negative(const StackValue &sv) { return negative_op(sv); }
Value not_op(const StackValue &sv) { return not_op_impl(sv); }

std::partial_ordering compare(const StackValue &a, const StackValue &b) {
  return compare_op(a, b);
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

  auto *gcv = container.get_heap_ptr();
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
