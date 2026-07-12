module l3.vm;

import l3.ast;
import utils;
import l3.runtime;

namespace l3::builtins {

namespace {
using l3::runtime::HeapData;
using l3::runtime::Primitive;
using l3::runtime::RuntimeError;
using l3::runtime::StackValue;
using l3::runtime::TypeError;
using l3::runtime::ValueError;

void format_args(
    const std::output_iterator<char> auto &out, l3::runtime::L3Args args
) {
  if (args.empty()) {
    return;
  }
  std::format_to(out, "{}", args[0]);
  for (const auto &arg : args | std::views::drop(1)) {
    std::format_to(out, " {}", arg);
  }
}

StackValue
builtin_print(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  format_args(std::ostream_iterator<char>(std::cout), args);
  return {};
}

StackValue builtin_println(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  builtin_print(vm, args);
  std::print("\n");
  return {};
}

StackValue
builtin_trigger_gc(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (!args.empty()) {
    throw RuntimeError("trigger_gc() takes no arguments");
  }
  vm.run_gc();
  return {};
}

StackValue
builtin_assert(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args[0].is_truthy()) {
    return {};
  }
  std::string result;
  format_args(std::back_inserter(result), args.subspan(1));
  throw RuntimeError("{}", result);
}

StackValue
builtin_error(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  std::string result;
  format_args(std::back_inserter(result), args);
  throw RuntimeError("{}", result);
}

StackValue builtin_input(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (!args.empty()) {
    builtin_print(vm, args);
  }
  std::string input;
  std::getline(std::cin, input);
  return vm.heap_store(std::move(input));
}

StackValue builtin_int(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args.empty()) {
    throw RuntimeError("int() takes at least one argument");
  }

  if (args.size() > 2) {
    throw RuntimeError("int() takes at most two arguments");
  }

  constexpr long INT_DEFAULT_BASE = 10;
  constexpr long INT_MAX_BASE = 36;
  constexpr long INT_MIN_BASE = 2;

  int base = INT_DEFAULT_BASE;
  if (args.size() == 2) {
    auto base_int = args[1].as_primitive().and_then(&Primitive::as_integer);
    if (!base_int) {
      throw RuntimeError("int() takes only an integer as a base argument");
    }

    if (*base_int < INT_MIN_BASE || *base_int > INT_MAX_BASE) {
      throw RuntimeError("int() takes a base between 2 and 36");
    }

    base = static_cast<int>(*base_int);
  }

  std::int64_t value = 0;

  const auto &arg = args[0];
  if (auto primitive_opt = arg.as_primitive()) {
    value = primitive_opt->get().visit([](const auto &value) {
      return static_cast<std::int64_t>(value);
    });
  } else if (auto string_opt = arg.as_string()) {
    const auto &string = string_opt->get();
    auto result =
        std::from_chars(&*string.begin(), &*string.end(), value, base);
    if (result.ec != std::errc{}) {
      throw RuntimeError(
          "invalid integer literal '{}' in base {}", string, base
      );
    }
  } else {
    throw RuntimeError("int() takes only primitive values or strings");
  }

  return {Primitive{value}};
}

StackValue builtin_str(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw RuntimeError("str() takes one argument");
  }

  std::string result;
  format_args(std::back_inserter(result), args);

  return vm.heap_store(std::move(result));
}

StackValue builtin_head(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.empty()) {
    throw RuntimeError("head() takes at least one argument");
  }

  const auto &argument = args[0];

  if (const auto &vector_opt = argument.as_vector()) {
    const auto &vec = vector_opt->get();
    if (vec.empty()) {
      throw RuntimeError("head() takes a non-empty vector");
    }

    auto head = vec.front();
    auto rest = std::vector<StackValue>(vec.begin() + 1, vec.end());

    return vm.heap_store(std::vector{head, vm.heap_store(std::move(rest))});
  }

  if (const auto &string_opt = argument.as_string()) {
    const auto &string = string_opt->get();

    if (string.empty()) {
      throw RuntimeError("head() takes a non-empty string");
    }

    auto head = vm.heap_store(std::string{string.front()});
    auto rest = vm.heap_store(std::string(string.substr(1)));

    return vm.heap_store(std::vector{head, rest});
  }

  throw TypeError("head() takes only vector and string values");
}

StackValue builtin_tail(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.empty()) {
    throw RuntimeError("tail() takes at least one argument");
  }

  const auto &argument = args[0];

  if (const auto &vector_opt = argument.as_vector()) {
    const auto &vec = vector_opt->get();
    if (vec.empty()) {
      throw RuntimeError("tail() takes a non-empty vector");
    }

    auto tail = vec.back();
    auto rest = std::vector<StackValue>(vec.begin(), vec.end() - 1);

    return vm.heap_store(std::vector{vm.heap_store(std::move(rest)), tail});
  }

  if (const auto &string_opt = argument.as_string()) {
    const auto &string = string_opt->get();

    if (string.empty()) {
      throw RuntimeError("tail() takes a non-empty string");
    }

    auto tail = vm.heap_store(std::string{string.back()});
    auto rest = vm.heap_store(std::string(string.substr(0, string.size() - 1)));

    return vm.heap_store(std::vector{rest, tail});
  }

  throw TypeError("tail() takes only vector and string values");
}

StackValue builtin_len(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw RuntimeError("len() takes exactly one argument");
  }

  const auto &arg = args[0];
  if (arg.is_string()) {
    return {
        Primitive{static_cast<std::int64_t>(arg.as_string()->get().size())}
    };
  }
  if (arg.is_vector()) {
    return {
        Primitive{static_cast<std::int64_t>(arg.as_vector()->get().size())}
    };
  }
  throw TypeError("len() does not support {} values");
}

StackValue builtin_drop(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 2) {
    throw RuntimeError("drop() takes two arguments");
  }

  auto index_opt = args[1].as_primitive().and_then(&Primitive::as_integer);
  if (!index_opt) {
    throw TypeError("drop() takes only an integer as an index argument");
  }
  auto index = *index_opt;

  return vm.heap_store(
      args[0].slice(l3::runtime::Slice{.start = index, .end = std::nullopt})
  );
}

StackValue builtin_take(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 2) {
    throw RuntimeError("take() takes two arguments");
  }

  auto index_opt = args[1].as_primitive().and_then(&Primitive::as_integer);
  if (!index_opt) {
    throw TypeError("take() takes only an integer as an index argument");
  }
  auto index = *index_opt;

  return vm.heap_store(
      args[0].slice(l3::runtime::Slice{.start = std::nullopt, .end = index})
  );
}

StackValue builtin_slice(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 3) {
    throw RuntimeError("slice() takes three arguments");
  }

  auto start_opt = args[1].as_primitive().and_then(&Primitive::as_integer);
  auto end_opt = args[2].as_primitive().and_then(&Primitive::as_integer);
  if (!start_opt || !end_opt) {
    throw TypeError("slice() takes only integers as index arguments");
  }
  auto start = *start_opt;
  auto end = *end_opt;

  return vm.heap_store(
      args[0].slice(l3::runtime::Slice{.start = start, .end = end})
  );
}

StackValue
builtin_random(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  static std::random_device rd;
  static std::mt19937 gen(rd());

  if (args.empty() || args.size() > 2) {
    throw RuntimeError("random() takes one or two arguments");
  }

  std::optional<std::int64_t> min_opt;
  std::optional<std::int64_t> max_opt;

  if (args.size() == 2) {
    min_opt = args[0].as_primitive().and_then(&Primitive::as_integer);
    max_opt = args[1].as_primitive().and_then(&Primitive::as_integer);
  } else {
    min_opt = std::make_optional(std::int64_t{0});
    max_opt = args[0].as_primitive().and_then(&Primitive::as_integer);
  }

  if (!min_opt || !max_opt) {
    throw TypeError("random() takes only integers as arguments");
  }

  auto min = *min_opt;
  auto max = *max_opt;

  auto distribution = std::uniform_int_distribution<std::int64_t>{min, max};

  return {Primitive{distribution(gen)}};
}

StackValue
builtin_sleep(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw RuntimeError("sleep() takes one argument");
  }
  auto duration_opt = args[0].as_primitive().and_then(&Primitive::as_integer);
  if (!duration_opt) {
    throw TypeError("sleep() takes only an integer as an duration argument");
  }
  auto duration = *duration_opt;
  std::this_thread::sleep_for(std::chrono::milliseconds(duration));
  return {};
}

StackValue builtin_map(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 2) {
    throw TypeError("map() takes exactly 2 arguments");
  }

  if (!args[0].is_function()) {
    throw TypeError("map() first argument must be a function");
  }

  const auto list_opt = args[1].as_vector();
  if (!list_opt) {
    throw TypeError("map() second argument must be a vector");
  }
  const auto &list = list_opt->get();

  auto result = std::views::transform(
                    list,
                    [&](const auto &item) {
                      return vm.call_function(args[0], std::array{item});
                    }
                ) |
                std::ranges::to<std::vector>();

  return vm.heap_store(std::move(result));
}

StackValue builtin_filter(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 2) {
    throw TypeError("filter() takes exactly 2 arguments");
  }

  if (!args[0].is_function()) {
    throw TypeError("filter() first argument must be a function");
  }

  const auto list_opt = args[1].as_vector();
  if (!list_opt) {
    throw TypeError("filter() second argument must be a vector");
  }
  const auto &list = list_opt->get();

  auto result = std::vector<StackValue>{};
  for (const auto &item : list) {
    if (vm.call_function(args[0], std::array{item}).is_truthy()) {
      result.push_back(item);
    }
  }

  return vm.heap_store(std::move(result));
}

StackValue builtin_sum(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw TypeError("sum() takes exactly 1 argument");
  }

  const auto list_opt = args[0].as_vector();
  if (!list_opt) {
    throw TypeError("sum() argument must be a vector");
  }
  const auto &list = list_opt->get();

  if (list.empty()) {
    throw TypeError("sum() cannot be applied to an empty vector");
  }

  HeapData total = l3::runtime::to_owned(list.front());
  for (const auto &item : list | std::views::drop(1)) {
    total.add_assign(l3::runtime::to_owned(item));
  }

  return vm.heap_store(std::move(total));
}

StackValue builtin_all(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw TypeError("all() takes exactly 1 argument");
  }

  const auto list_opt = args[0].as_vector();
  if (!list_opt) {
    throw TypeError("all() argument must be a vector");
  }
  const auto &list = list_opt->get();

  for (const auto &item : list) {
    if (!item.is_truthy()) {
      return {Primitive{false}};
    }
  }

  return {Primitive{true}};
}

StackValue builtin_any(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw TypeError("any() takes exactly 1 argument");
  }

  const auto list_opt = args[0].as_vector();
  if (!list_opt) {
    throw TypeError("any() argument must be a vector");
  }
  const auto &list = list_opt->get();

  for (const auto &item : list) {
    if (item.is_truthy()) {
      return {Primitive{true}};
    }
  }

  return {Primitive{false}};
}

StackValue builtin_count(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 2) {
    throw TypeError("count() takes exactly 2 arguments");
  }

  if (!args[0].is_function()) {
    throw TypeError("count() first argument must be a function");
  }

  const auto list_opt = args[1].as_vector();
  if (!list_opt) {
    throw TypeError("count() second argument must be a vector");
  }
  const auto &list = list_opt->get();

  std::int64_t count = 0;
  for (const auto &item : list) {
    if (vm.call_function(args[0], std::array{item}).is_truthy()) {
      ++count;
    }
  }

  return {Primitive{count}};
}

StackValue
builtin_identity(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw TypeError("id() takes exactly 1 argument");
  }
  return args[0];
}

StackValue builtin_range(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  std::int64_t start = 0;
  std::int64_t end = 0;
  std::int64_t step = 1;

  switch (args.size()) {
  case 1:
    start = 0;
    end = args[0].as_primitive().and_then(&Primitive::as_integer).value();
    break;
  case 2:
    start = args[0].as_primitive().and_then(&Primitive::as_integer).value();
    end = args[1].as_primitive().and_then(&Primitive::as_integer).value();
    break;
  case 3:
    start = args[0].as_primitive().and_then(&Primitive::as_integer).value();
    end = args[1].as_primitive().and_then(&Primitive::as_integer).value();
    step = args[2].as_primitive().and_then(&Primitive::as_integer).value();
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

  auto result = std::vector<StackValue>{};
  for (std::int64_t i = start; step > 0 ? i < end : i > end; i += step) {
    result.push_back(vm.heap_store(Primitive{i}));
  }
  return vm.heap_store(std::move(result));
}

} // namespace

const std::initializer_list<std::pair<
    std::string_view,
    runtime::StackValue (*)(l3::vm::BytecodeVM &, runtime::L3Args)>>
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

} // namespace l3::builtins
