#include "vm/scope.hpp"
#include "vm/error.hpp"
#include "vm/format.hpp"
#include "vm/function.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"
#include <functional>
#include <optional>
#include <print>
#include <ranges>
#include <string>
#include <utility>
#include <utils/format.h>

namespace l3::vm {

utils::optional_cref<Variable> Scope::get_variable(const Identifier &id) const {
  auto present = variables.find(id);
  if (present == variables.end()) {
    return std::nullopt;
  }
  return std::optional{std::cref(present->second)};
}
utils::optional_ref<Variable> Scope::get_variable_mut(const Identifier &id) {
  auto present = variables.find(id);
  if (present == variables.end()) {
    return std::nullopt;
  }
  return std::optional{std::ref(present->second)};
}

Variable &Scope::declare_variable(
    const Identifier &id, RefValue ref_value, Mutability mut
) {
  const auto present = variables.find(id);
  if (present != variables.end()) {
    throw NameError("variable '{}' already declared", id.get_name());
  }
  auto &[_, value] =
      *variables.emplace_hint(present, id, Variable{ref_value, mut});
  return value;
}

namespace {

std::pair<Identifier, GCValue>
wrap_native_function(std::string_view name, BuiltinFunction::Body function) {
  return {
      Identifier{name},
      GCValue{Value{BuiltinFunction{Identifier{name}, std::move(function)}}}
  };
}

void format_args(const std::output_iterator<char> auto &out, L3Args args) {
  if (args.empty()) {
    return;
  }
  std::format_to(out, "{}", ValuePrettyPrinter(*args[0]));
  for (const auto &arg : args | std::views::drop(1)) {
    std::format_to(out, " {}", ValuePrettyPrinter(*arg));
  }
}

RefValue print(VM & /*vm*/, L3Args args) {
  format_args(std::ostream_iterator<char>(std::cout), args);
  return VM::nil();
}

RefValue println(VM &vm, L3Args args) {
  print(vm, args);
  std::print("\n");
  return VM::nil();
}

RefValue trigger_gc(VM &vm, L3Args args) {
  if (args.size() > 0) {
    throw RuntimeError("trigger_gc takes no arguments");
  }
  vm.run_gc();
  return VM::nil();
}

RefValue l3_assert(VM & /*vm*/, L3Args args) {
  if (args[0]->is_truthy()) {
    return VM::nil();
  }
  std::string result;
  format_args(std::back_inserter(result), args.subspan(1));
  throw RuntimeError("{}", result);
}

RefValue error(VM & /*vm*/, L3Args args) {
  std::string result;
  format_args(std::back_inserter(result), args);
  throw RuntimeError("{}", result);
}

RefValue input(VM &vm, L3Args args) {
  if (args.size() > 0) {
    println(vm, args);
  }
  std::string input;
  std::getline(std::cin, input);
  return vm.store_value(Value{Primitive{std::move(input)}});
}

RefValue to_int(VM &vm, L3Args args) {
  if (args.empty()) {
    throw RuntimeError("to_int takes at least one arguments");
  }

  auto primitive = args[0]->as_primitive();
  if (!primitive) {
    throw RuntimeError("to_int takes only primitive values");
  }

  if (args.size() > 2) {
    throw RuntimeError("to_int takes at most two arguments");
  }

  auto base_value =
      args.size() == 2 ? std::make_optional(args[1]) : std::nullopt;

  int base = 0;
  if (!base_value) {
    base = 10;
  } else {
    auto base_primitive =
        base_value.value()->as_primitive().and_then(&Primitive::as_integer);

    if (!base_primitive) {
      throw RuntimeError("to_int takes only an integer as a base argument");
    }

    if (*base_primitive < 2 || *base_primitive > 36) {
      throw RuntimeError("to_int takes a base between 2 and 36");
    }

    base = static_cast<int>(*base_primitive);
  }

  auto value = primitive->get().visit(
      [](const std::int64_t &integer) { return integer; },
      [base](const std::string &string) {
        std::int64_t value = 0;

        if (std::from_chars(string.data(), &*string.end(), value, base)) {
          return value;
        }

        throw RuntimeError(
            "invalid integer literal '{}' in base {}", string, base
        );
      },
      [](const auto &value) { return static_cast<std::int64_t>(value); }
  );

  return vm.store_value(Value{Primitive{value}});
}

RefValue to_string(VM &vm, L3Args args) {
  if (args.size() != 1) {
    throw RuntimeError("to_string takes one arguments");
  }

  std::string result;
  format_args(std::back_inserter(result), args);

  return vm.store_value(Value{Primitive{std::move(result)}});
}

RefValue head(VM &vm, L3Args args) {
  if (args.empty()) {
    throw RuntimeError("head takes at least one arguments");
  }

  auto argument = args[0];

  if (const auto &vector_opt = argument->as_vector()) {
    const auto &vector = vector_opt->get();

    if (vector.empty()) {
      throw RuntimeError("head takes a non-empty vector");
    }

    auto head = vector.front();
    auto rest = vm.store_new_value(
        Value{Value::vector_type{vector.begin() + 1, vector.end()}}
    );

    return vm.store_new_value(Value{Value::vector_type{head, rest}});
  }

  if (const auto &string_opt =
          argument->as_primitive().and_then(&Primitive::as_string);
      string_opt.has_value()) {
    const auto &string = string_opt->get();

    if (string.empty()) {
      throw RuntimeError("head takes a non-empty string");
    }

    auto head = vm.store_new_value({Primitive{std::string{string.front()}}});
    auto rest = vm.store_new_value({Primitive{string.substr(1)}});

    return vm.store_new_value({Value::vector_type{head, rest}});
  }

  throw TypeError("head takes only vector and string values");
}

RefValue tail(VM &vm, L3Args args) {
  if (args.empty()) {
    throw RuntimeError("tail takes at least one arguments");
  }

  auto argument = args[0];

  if (const auto &vector_opt = argument->as_vector()) {
    const auto &vector = vector_opt->get();

    if (vector.empty()) {
      throw RuntimeError("tail takes a non-empty vector");
    }

    auto tail = vector.back();
    auto rest = vm.store_new_value(
        Value{Value::vector_type{vector.begin(), vector.end() - 1}}
    );

    return vm.store_new_value(Value{Value::vector_type{rest, tail}});
  }

  if (const auto &string_opt =
          argument->as_primitive().and_then(&Primitive::as_string);
      string_opt.has_value()) {
    const auto &string = string_opt->get();

    if (string.empty()) {
      throw RuntimeError("tail takes a non-empty string");
    }

    auto tail = vm.store_new_value({Primitive{std::string{string.back()}}});
    auto rest =
        vm.store_new_value({Primitive{string.substr(0, string.size() - 1)}});

    return vm.store_new_value({Value::vector_type{rest, tail}});
  }

  throw TypeError("tail takes only vector and string values");
}

RefValue len(VM &vm, L3Args args) {
  if (args.size() != 1) {
    throw RuntimeError("len takes exactly one arguments");
  }
  return args[0]->visit(
      [&vm](const Value::vector_type &vector) -> RefValue {
        return vm.store_value(
            Value{Primitive{static_cast<std::int64_t>(vector.size())}}
        );
      },
      [&vm](const Primitive &primitive) -> RefValue {
        if (const auto &string = primitive.as_string()) {
          return vm.store_value(
              Value{Primitive{static_cast<std::int64_t>(string->get().size())}}
          );
        }
        throw TypeError(
            "len does not support {} values", primitive.type_name()
        );
      },
      [](const auto &) -> RefValue {
        throw TypeError("len does not support {} values");
      }
  );
}

Scope::BuiltinsMap create_builtins() {
  Scope::BuiltinsMap builtins;

  for (const auto &[name, value] :
       std::initializer_list<std::pair<std::string, BuiltinFunction::Body>>{
           {"print", print},
           {"println", println},
           {"error", error},
           {"assert", l3_assert},
           {"input", input},
           {"__trigger_gc", trigger_gc},
           {"int", to_int},
           {"str", to_string},
           {"head", head},
           {"tail", tail},
           {"len", len}
       }) {
    builtins.emplace(wrap_native_function(name, value));
  }

  return builtins;
}

} // namespace

Scope::BuiltinsMap Scope::builtins = create_builtins();

std::optional<RefValue> Scope::get_builtin(const Identifier &id) {
  auto present = builtins.find(id);
  if (present == builtins.end()) {
    return std::nullopt;
  }
  return RefValue{present->second};
}

Scope::Scope(VariableMap &&variables) : variables{std::move(variables)} {};
// const Scope &Scope::builtins() { return _builtins; }

void Scope::mark_gc() {
  for (auto &[name, it] : variables) {
    it->get_gc_mut().mark();
  }
}
bool Scope::has_variable(const Identifier &id) const {
  return variables.contains(id);
}

} // namespace l3::vm
