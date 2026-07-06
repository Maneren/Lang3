module l3.runtime;

import :gc_value;

namespace l3::runtime {

StackValue::StackValue() : inner{Nil{}} {}
StackValue::StackValue(Nil /*unused*/) : inner{Nil{}} {}
StackValue::StackValue(Primitive primitive) : inner{primitive} {}
StackValue::StackValue(GCValue *gc_value) : inner{gc_value} {}

bool StackValue::is_nil() const { return std::holds_alternative<Nil>(inner); }

bool StackValue::is_primitive() const {
  return std::holds_alternative<Primitive>(inner);
}

bool StackValue::is_truthy() const {
  return visit(
      [](Nil) { return false; },
      [](const Primitive &p) -> bool { return p.is_truthy(); },
      [](GCValue *gcv) -> bool {
        if (!gcv) {
          return false;
        }
        return gcv->get_value().visit(
            [](const std::string &s) { return !s.empty(); },
            [](const std::vector<StackValue> &v) { return !v.empty(); },
            [](const auto &) -> bool { return true; }
        );
      }
  );
}

std::string_view StackValue::type_name() const {
  using std::string_view_literals::operator""sv;
  return visit(
      [](const Primitive &p) { return p.type_name(); },
      [](Nil) { return "nil"sv; },
      [](GCValue *gcv) -> std::string_view {
        if (!gcv) {
          return "nil"sv;
        }
        return gcv->get_value().visit(
            [](const Value::function_type &) { return "function"sv; },
            [](const std::vector<StackValue> &) { return "vector"sv; },
            [](const std::string &) { return "string"sv; },
            [](const Primitive &p) { return p.type_name(); },
            [](Nil) { return "nil"sv; }
        );
      }
  );
}

utils::optional_cref<Primitive> StackValue::as_primitive() const {
  if (const auto *p = std::get_if<Primitive>(&inner)) {
    return *p;
  }
  return std::nullopt;
}

bool StackValue::is_string() const {
  if (auto *gcv = get_gc_ptr()) {
    return gcv->get_value().is_string();
  }
  return false;
}

bool StackValue::is_vector() const {
  if (auto *gcv = get_gc_ptr()) {
    return gcv->get_value().is_vector();
  }
  return false;
}

bool StackValue::is_function() const {
  if (auto *gcv = get_gc_ptr()) {
    return gcv->get_value().is_function();
  }
  return false;
}

utils::optional_cref<std::string> StackValue::as_string() const {
  if (auto *gcv = get_gc_ptr()) {
    return gcv->get_value().as_string();
  }
  return std::nullopt;
}

utils::optional_cref<std::vector<StackValue>> StackValue::as_vector() const {
  if (auto *gcv = get_gc_ptr()) {
    return gcv->get_value().as_vector();
  }
  return std::nullopt;
}

utils::optional_ref<std::vector<StackValue>> StackValue::as_mut_vector() {
  if (auto *gcv = get_gc_ptr()) {
    return gcv->get_value_mut().as_mut_vector();
  }
  return std::nullopt;
}

Value StackValue::slice(Slice slice) const {
  if (auto *gcv = get_gc_ptr()) {
    return gcv->get_value().slice(slice);
  }
  throw RuntimeError("slice() requires a string or vector");
}

} // namespace l3::runtime
