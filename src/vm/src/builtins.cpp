module;

#include <cstdint>
#include <iostream>
#include <optional>
#include <print>
#include <random>
#include <ranges>
#include <thread>

module l3.vm;

import l3.ast;
import utils;
import :variable;
import :formatting;

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

RefValue builtin_print(VM & /*vm*/, L3Args args) {
  format_args(std::ostream_iterator<char>(std::cout), args);
  return VM::nil();
}

RefValue builtin_println(VM &vm, L3Args args) {
  builtin_print(vm, args);
  std::print("\n");
  return VM::nil();
}

RefValue builtin_trigger_gc(VM &vm, L3Args args) {
  if (args.size() > 0) {
    throw RuntimeError("trigger_gc() takes no arguments");
  }
  vm.run_gc();
  return VM::nil();
}

RefValue builtin_assert(VM & /*vm*/, L3Args args) {
  if (args[0]->is_truthy()) {
    return VM::nil();
  }
  std::string result;
  format_args(std::back_inserter(result), args.subspan(1));
  throw RuntimeError("{}", result);
}

RefValue builtin_error(VM & /*vm*/, L3Args args) {
  std::string result;
  format_args(std::back_inserter(result), args);
  throw RuntimeError("{}", result);
}

RefValue builtin_input(VM &vm, L3Args args) {
  if (args.size() > 0) {
    builtin_print(vm, args);
  }
  std::string input;
  std::getline(std::cin, input);
  return vm.store_value({std::move(input)});
}

RefValue builtin_int(VM &vm, L3Args args) {
  if (args.empty()) {
    throw RuntimeError("int() takes at least one arguments");
  }

  if (args.size() > 2) {
    throw RuntimeError("int() takes at most two arguments");
  }

  int base = 10;
  if (args.size() == 2) {
    auto base_int = args[1]->as_primitive().and_then(&Primitive::as_integer);
    if (!base_int) {
      throw RuntimeError("int() takes only an integer as a base argument");
    }

    if (*base_int < 2 || *base_int > 36) {
      throw RuntimeError("int() takes a base between 2 and 36");
    }

    base = static_cast<int>(*base_int);
  }

  std::int64_t value = 0;

  const auto &arg = args[0];
  if (auto primitive_opt = arg->as_primitive()) {
    value = primitive_opt->get().visit([](const auto &value) {
      return static_cast<std::int64_t>(value);
    });
  } else if (auto string_opt = arg->as_string()) {
    const auto &string = string_opt->get();
    auto result = std::from_chars(
        string.data(), string.data() + string.size(), value, base
    );
    if (result.ec != std::errc{}) {
      throw RuntimeError(
          "invalid integer literal '{}' in base {}", string, base
      );
    }
  } else {
    throw RuntimeError("int() takes only primitive values or strings");
  }

  return vm.store_value(Primitive{value});
}

RefValue builtin_str(VM &vm, L3Args args) {
  if (args.size() != 1) {
    throw RuntimeError("str() takes one arguments");
  }

  std::string result;
  format_args(std::back_inserter(result), args);

  return vm.store_value({std::move(result)});
}

RefValue builtin_head(VM &vm, L3Args args) {
  if (args.empty()) {
    throw RuntimeError("head() takes at least one arguments");
  }

  auto argument = args[0];

  if (const auto &vector_opt = argument->as_vector()) {
    const auto &vector = vector_opt->get();

    if (vector.empty()) {
      throw RuntimeError("head() takes a non-empty vector");
    }

    auto head = vector.front();
    auto rest =
        vm.store_value({Value::vector_type{vector.begin() + 1, vector.end()}});

    return vm.store_value({Value::vector_type{head, rest}});
  }

  if (const auto &string_opt = argument->as_string()) {
    const auto &string = string_opt->get();

    if (string.empty()) {
      throw RuntimeError("head() takes a non-empty string");
    }

    auto head = vm.store_new_value({std::string{string.front()}});
    auto rest = vm.store_new_value({string.substr(1)});

    return vm.store_new_value({Value::vector_type{head, rest}});
  }

  throw TypeError("head() takes only vector and string values");
}

RefValue builtin_tail(VM &vm, L3Args args) {
  if (args.empty()) {
    throw RuntimeError("tail() takes at least one arguments");
  }

  auto argument = args[0];

  if (const auto &vector_opt = argument->as_vector()) {
    const auto &vector = vector_opt->get();

    if (vector.empty()) {
      throw RuntimeError("tail() takes a non-empty vector");
    }

    auto tail = vector.back();
    auto rest =
        vm.store_value({Value::vector_type{vector.begin(), vector.end() - 1}});

    return vm.store_value({Value::vector_type{rest, tail}});
  }

  if (const auto &string_opt = argument->as_string()) {
    const auto &string = string_opt->get();

    if (string.empty()) {
      throw RuntimeError("tail() takes a non-empty string");
    }

    auto tail = vm.store_value({std::string{string.back()}});
    auto rest = vm.store_value({string.substr(0, string.size() - 1)});

    return vm.store_value({Value::vector_type{rest, tail}});
  }

  throw TypeError("tail() takes only vector and string values");
}

RefValue builtin_len(VM &vm, L3Args args) {
  if (args.size() != 1) {
    throw RuntimeError("len() takes exactly one arguments");
  }
  return args[0]->visit(
      [&vm](const ValueContainer auto &container) -> RefValue {
        return vm.store_value(
            {Primitive{static_cast<std::int64_t>(container.size())}}
        );
      },
      [](const auto &) -> RefValue {
        throw TypeError("len() does not support {} values");
      }
  );
}

RefValue builtin_drop(VM &vm, L3Args args) {
  if (args.size() != 2) {
    throw RuntimeError("drop() takes two arguments");
  }

  auto index_opt = args[1]->as_primitive().and_then(&Primitive::as_integer);
  if (!index_opt) {
    throw TypeError("drop() takes only an integer as an index argument");
  }
  auto index = *index_opt;

  return vm.store_value(
      args[0]->slice(Slice{.start = index, .end = std::nullopt})
  );
}

RefValue builtin_take(VM &vm, L3Args args) {
  if (args.size() != 2) {
    throw RuntimeError("take() takes two arguments");
  }

  auto index_opt = args[1]->as_primitive().and_then(&Primitive::as_integer);
  if (!index_opt) {
    throw TypeError("take() takes only an integer as an index argument");
  }
  auto index = *index_opt;

  return vm.store_value(
      args[0]->slice(Slice{.start = std::nullopt, .end = index})
  );
}

RefValue builtin_slice(VM &vm, L3Args args) {
  if (args.size() != 3) {
    throw RuntimeError("slice() takes three arguments");
  }

  auto start_opt = args[1]->as_primitive().and_then(&Primitive::as_integer);
  auto end_opt = args[2]->as_primitive().and_then(&Primitive::as_integer);
  if (!start_opt || !end_opt) {
    throw TypeError("slice() takes only integers as index arguments");
  }
  auto start = *start_opt;
  auto end = *end_opt;

  return vm.store_value(args[0]->slice(Slice{.start = start, .end = end}));
}

RefValue builtin_random(VM &vm, L3Args args) {
  static std::random_device rd;
  static std::mt19937 gen(rd());

  if (args.empty() || args.size() > 2) {
    throw RuntimeError("random() takes one or two arguments");
  }

  std::optional<std::int64_t> min_opt;
  std::optional<std::int64_t> max_opt;

  if (args.size() == 2) {
    min_opt = args[0]->as_primitive().and_then(&Primitive::as_integer);
    max_opt = args[1]->as_primitive().and_then(&Primitive::as_integer);
  } else {
    min_opt = std::make_optional(std::int64_t{0});
    max_opt = args[0]->as_primitive().and_then(&Primitive::as_integer);
  }

  if (!min_opt || !max_opt) {
    throw TypeError("random() takes only integers as arguments");
  }

  auto min = *min_opt;
  auto max = *max_opt;

  auto distribution = std::uniform_int_distribution<std::int64_t>{min, max};

  return vm.store_value(Value{Primitive{distribution(gen)}});
}

RefValue builtin_sleep(VM & /*vm*/, L3Args args) {
  if (args.size() != 1) {
    throw RuntimeError("sleep() takes one argument");
  }
  auto duration_opt = args[0]->as_primitive().and_then(&Primitive::as_integer);
  if (!duration_opt) {
    throw TypeError("sleep() takes only an integer as an duration argument");
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

  return vm.store_value(
      {list | std::views::transform([&vm, &func](const auto &item) {
         return func(vm, {item});
       }) |
       std::ranges::to<std::vector>()}
  );
}

RefValue builtin_filter(VM &vm, L3Args args) {
  if (args.size() != 2) {
    throw TypeError("filter() takes exactly 2 arguments");
  }

  auto func_opt = args[0]->as_function();
  if (!func_opt) {
    throw TypeError("filter() first argument must be a function");
  }
  auto &func = *func_opt->get();

  const auto list_opt = args[1]->as_vector();
  if (!list_opt) {
    throw TypeError("filter() second argument must be a vector");
  }
  const auto &list = list_opt->get();

  return vm.store_value(
      {list | std::views::filter([&vm, &func](const auto &item) {
         return func(vm, {item})->is_truthy();
       }) |
       std::ranges::to<std::vector>()}
  );
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

RefValue builtin_count(VM &vm, L3Args args) {
  if (args.size() != 2) {
    throw TypeError("count() takes exactly 2 arguments");
  }

  const auto fn_opt = args[0]->as_function();
  if (!fn_opt) {
    throw TypeError("count() first argument must be a function");
  }
  auto &fn = *fn_opt->get();

  const auto list_opt = args[1]->as_vector();
  if (!list_opt) {
    throw TypeError("count() second argument must be a vector");
  }
  const auto &list = list_opt->get();

  std::int64_t count = 0;
  for (const auto &item : list) {
    if (fn(vm, {item})->is_truthy()) {
      ++count;
    }
  }

  return vm.store_value(Primitive{count});
}

RefValue builtin_identity(VM & /*vm*/, L3Args args) {
  if (args.size() != 1) {
    throw TypeError("id() takes exactly 1 argument");
  }
  return args[0];
}

RefValue builtin_range(VM &vm, L3Args args) {
  std::int64_t start = 0;
  std::int64_t end = 0;
  std::int64_t step = 1;

  switch (args.size()) {
  case 1:
    start = 0;
    end = args[0]->as_primitive().and_then(&Primitive::as_integer).value();
    break;
  case 2:
    start = args[0]->as_primitive().and_then(&Primitive::as_integer).value();
    end = args[1]->as_primitive().and_then(&Primitive::as_integer).value();
    break;
  case 3:
    start = args[0]->as_primitive().and_then(&Primitive::as_integer).value();
    end = args[1]->as_primitive().and_then(&Primitive::as_integer).value();
    step = args[2]->as_primitive().and_then(&Primitive::as_integer).value();
    break;
  default:
    throw TypeError("range() takes 1, 2 or 3 arguments");
  }

  if (step == 0) {
    throw ValueError("range() step cannot be 0");
  }

  if (step > 0) {
    if (start > end) {
      throw ValueError("range() start > end");
    }
  } else {
    if (start < end) {
      throw ValueError("range() start < end with negative step");
    }
  }

  Value::vector_type result;
  for (std::int64_t i = start; step > 0 ? i < end : i > end; i += step) {
    result.push_back(vm.store_value(Primitive{i}));
  }
  return vm.store_value(std::move(result));
}

} // namespace

extern const std::initializer_list<
    std::pair<std::string_view, BuiltinFunction::Body>>
    BUILTINS{
        {"print", builtin_print},
        {"println", builtin_println},
        {"__trigger_gc", builtin_trigger_gc},
        {"assert", builtin_assert},
        {"error", builtin_error},
        {"input", builtin_input},
        {"int", builtin_int},
        {"str", builtin_str},
        {"head", builtin_head},
        {"tail", builtin_tail},
        {"len", builtin_len},
        {"drop", builtin_drop},
        {"take", builtin_take},
        {"slice", builtin_slice},
        {"random", builtin_random},
        {"sleep", builtin_sleep},
        {"map", builtin_map},
        {"filter", builtin_filter},
        {"sum", builtin_sum},
        {"all", builtin_all},
        {"any", builtin_any},
        {"count", builtin_count},
        {"id", builtin_identity},
        {"range", builtin_range},
    };

} // namespace l3::vm::builtins
