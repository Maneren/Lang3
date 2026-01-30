#include "vm/builtins.hpp"
#include "vm/error.hpp"
#include "vm/format.hpp"
#include "vm/vm.hpp"
#include <random>
#include <ranges>
#include <thread>

namespace l3::vm::builtins {

namespace {

void format_args(const std::output_iterator<char> auto &out, L3Args args) {
  if (args.empty()) {
    return;
  }
  std::format_to(out, "{}", ValuePrettyPrinter(*args[0]));
  for (const auto &arg : args | std::views::drop(1)) {
    std::format_to(out, " {}", ValuePrettyPrinter(*arg));
  }
}

} // namespace

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
  return vm.store_value({Primitive{std::move(input)}});
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

  return vm.store_value({Primitive{value}});
}

RefValue to_string(VM &vm, L3Args args) {
  if (args.size() != 1) {
    throw RuntimeError("to_string takes one arguments");
  }

  std::string result;
  format_args(std::back_inserter(result), args);

  return vm.store_value({Primitive{std::move(result)}});
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
    auto rest =
        vm.store_value({Value::vector_type{vector.begin() + 1, vector.end()}});

    return vm.store_value({Value::vector_type{head, rest}});
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
    auto rest =
        vm.store_value({Value::vector_type{vector.begin(), vector.end() - 1}});

    return vm.store_value({Value::vector_type{rest, tail}});
  }

  if (const auto &string_opt =
          argument->as_primitive().and_then(&Primitive::as_string);
      string_opt.has_value()) {
    const auto &string = string_opt->get();

    if (string.empty()) {
      throw RuntimeError("tail takes a non-empty string");
    }

    auto tail = vm.store_value({Primitive{std::string{string.back()}}});
    auto rest =
        vm.store_value({Primitive{string.substr(0, string.size() - 1)}});

    return vm.store_value({Value::vector_type{rest, tail}});
  }

  throw TypeError("tail takes only vector and string values");
}

RefValue len(VM &vm, L3Args args) {
  if (args.size() != 1) {
    throw RuntimeError("len takes exactly one arguments");
  }
  return args[0]->visit(
      [&vm](const Value::vector_ptr_type &vector) -> RefValue {
        return vm.store_value(
            {Primitive{static_cast<std::int64_t>(vector->size())}}
        );
      },
      [&vm](const Primitive &primitive) -> RefValue {
        if (const auto &string = primitive.as_string()) {
          return vm.store_value(
              {Primitive{static_cast<std::int64_t>(string->get().size())}}
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

RefValue drop(VM &vm, L3Args args) {
  if (args.size() != 2) {
    throw RuntimeError("drop takes two arguments");
  }

  auto index_opt = args[1]->as_primitive().and_then(&Primitive::as_integer);
  if (!index_opt) {
    throw TypeError("drop takes only an integer as an index argument");
  }
  auto index = *index_opt;

  return vm.store_value(args[0]->slice(Slice{index, std::nullopt}));
}

RefValue take(VM &vm, L3Args args) {
  if (args.size() != 2) {
    throw RuntimeError("take takes two arguments");
  }

  auto index_opt = args[1]->as_primitive().and_then(&Primitive::as_integer);
  if (!index_opt) {
    throw TypeError("take takes only an integer as an index argument");
  }
  auto index = *index_opt;

  return vm.store_value(args[0]->slice(Slice{std::nullopt, index}));
}

RefValue slice(VM &vm, L3Args args) {
  if (args.size() != 3) {
    throw RuntimeError("slice takes three arguments");
  }

  auto start_opt = args[1]->as_primitive().and_then(&Primitive::as_integer);
  auto end_opt = args[2]->as_primitive().and_then(&Primitive::as_integer);
  if (!start_opt || !end_opt) {
    throw TypeError("slice takes only integers as index arguments");
  }
  auto start = *start_opt;
  auto end = *end_opt;

  return vm.store_value(args[0]->slice(Slice{start, end}));
}

RefValue random(VM &vm, L3Args args) {
  static std::random_device rd;
  static std::mt19937 gen(rd());

  if (args.empty() || args.size() > 2) {
    throw RuntimeError("random takes one or two arguments");
  }

  std::optional<std::int64_t> min_opt, max_opt;

  if (args.size() == 2) {
    min_opt = args[0]->as_primitive().and_then(&Primitive::as_integer);
    max_opt = args[1]->as_primitive().and_then(&Primitive::as_integer);
  } else {
    min_opt = std::make_optional(std::int64_t{0});
    max_opt = args[0]->as_primitive().and_then(&Primitive::as_integer);
  }

  if (!min_opt || !max_opt) {
    throw TypeError("random takes only integers as arguments");
  }

  auto min = *min_opt;
  auto max = *max_opt;

  auto distribution = std::uniform_int_distribution<std::int64_t>{min, max};

  return vm.store_value(Value{Primitive{distribution(gen)}});
}

RefValue sleep(VM & /*vm*/, L3Args args) {
  if (args.size() != 1) {
    throw RuntimeError("sleep takes one argument");
  }
  auto duration_opt = args[0]->as_primitive().and_then(&Primitive::as_integer);
  if (!duration_opt) {
    throw TypeError("sleep takes only an integer as an duration argument");
  }
  auto duration = *duration_opt;
  std::this_thread::sleep_for(std::chrono::milliseconds(duration));
  return VM::nil();
}

RefValue builtin_map(VM &vm, L3Args args) {
  if (args.size() != 2) {
    throw TypeError("map() takes exactly 2 arguments");
  }

  auto func_opt = args[0]->as_function();
  if (!func_opt) {
    throw TypeError("map() first argument must be a function");
  }
  auto &func = *func_opt->get();

  const auto list_opt = args[1]->as_vector();
  if (!list_opt) {
    throw TypeError("map() second argument must be a vector");
  }
  const auto &list = list_opt->get();

  std::vector<RefValue> result;
  std::vector<RefValue> func_args;
  result.reserve(list.size());
  for (const auto &item : list) {
    func_args.push_back(item);
    auto mapped_value = func(vm, func_args);
    func_args.pop_back();
    result.push_back(mapped_value);
  }

  return vm.store_value(Value{std::move(result)});
}

RefValue builtin_sum(VM &vm, L3Args args) {
  if (args.size() != 1) {
    throw TypeError("sum() takes exactly 1 argument");
  }

  const auto list_opt = args[0]->as_vector();
  if (!list_opt) {
    throw TypeError("sum() argument must be a vector");
  }
  const auto &list = list_opt->get();

  if (list.empty()) {
    throw TypeError("sum() cannot be applied to an empty vector");
  }

  RefValue total = list.front();
  for (auto item : list | std::views::drop(1)) {
    total = vm.store_value(total->add(*item));
  }

  return total;
}

RefValue builtin_all(VM & /*vm*/, L3Args args) {
  if (args.size() != 1) {
    throw TypeError("all() takes exactly 1 argument");
  }

  const auto list_opt = args[0]->as_vector();
  if (!list_opt) {
    throw TypeError("all() argument must be a vector");
  }
  const auto &list = list_opt->get();

  for (auto item : list) {
    if (item->is_falsy()) {
      return VM::_false();
    }
  }

  return VM::_true();
}

RefValue builtin_any(VM & /*vm*/, L3Args args) {
  if (args.size() != 1) {
    throw TypeError("any() takes exactly 1 argument");
  }

  const auto list_opt = args[0]->as_vector();
  if (!list_opt) {
    throw TypeError("any() argument must be a vector");
  }
  const auto &list = list_opt->get();

  for (auto item : list) {
    if (item->is_truthy()) {
      return VM::_true();
    }
  }

  return VM::_false();
}

const std::initializer_list<std::pair<std::string_view, BuiltinFunction::Body>>
    BUILTINS{
        {"print", print},     {"println", println},
        {"error", error},     {"assert", l3_assert},
        {"input", input},     {"__trigger_gc", trigger_gc},
        {"int", to_int},      {"str", to_string},
        {"head", head},       {"tail", tail},
        {"len", len},         {"drop", drop},
        {"take", take},       {"slice", slice},
        {"random", random},   {"sleep", sleep},
        {"map", builtin_map}, {"sum", builtin_sum},
        {"all", builtin_all}, {"any", builtin_any},
    };

} // namespace l3::vm::builtins
