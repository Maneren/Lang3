module l3.vm;

import l3.ast;
import utils;
import l3.runtime;

namespace l3::builtins {

namespace {

void format_args(
    const std::output_iterator<char> auto &out, l3::runtime::L3Args args
) {
  if (args.empty()) {
    return;
  }
  std::format_to(out, "{}", l3::runtime::ValuePrettyPrinter(*args[0]));
  for (const auto &arg : args | std::views::drop(1)) {
    std::format_to(out, " {}", l3::runtime::ValuePrettyPrinter(*arg));
  }
}

l3::runtime::Ref
builtin_print(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  format_args(std::ostream_iterator<char>(std::cout), args);
  return l3::runtime::Ref{l3::runtime::GCStorage::nil()};
}

l3::runtime::Ref
builtin_println(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  builtin_print(vm, args);
  std::print("\n");
  return l3::runtime::Ref{l3::runtime::GCStorage::nil()};
}

l3::runtime::Ref
builtin_trigger_gc(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (!args.empty()) {
    throw l3::runtime::RuntimeError("trigger_gc() takes no arguments");
  }
  vm.run_gc();
  return l3::runtime::Ref{l3::runtime::GCStorage::nil()};
}

l3::runtime::Ref
builtin_assert(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args[0]->is_truthy()) {
    return l3::runtime::Ref{l3::runtime::GCStorage::nil()};
  }
  std::string result;
  format_args(std::back_inserter(result), args.subspan(1));
  throw l3::runtime::RuntimeError("{}", result);
}

l3::runtime::Ref
builtin_error(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  std::string result;
  format_args(std::back_inserter(result), args);
  throw l3::runtime::RuntimeError("{}", result);
}

l3::runtime::Ref
builtin_input(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (!args.empty()) {
    builtin_print(vm, args);
  }
  std::string input;
  std::getline(std::cin, input);
  return vm.store_value({std::move(input)});
}

l3::runtime::Ref builtin_int(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.empty()) {
    throw l3::runtime::RuntimeError("int() takes at least one arguments");
  }

  if (args.size() > 2) {
    throw l3::runtime::RuntimeError("int() takes at most two arguments");
  }

  int base = 10;
  if (args.size() == 2) {
    auto base_int =
        args[1]->as_primitive().and_then(&l3::runtime::Primitive::as_integer);
    if (!base_int) {
      throw l3::runtime::RuntimeError(
          "int() takes only an integer as a base argument"
      );
    }

    if (*base_int < 2 || *base_int > 36) {
      throw l3::runtime::RuntimeError("int() takes a base between 2 and 36");
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
      throw l3::runtime::RuntimeError(
          "invalid integer literal '{}' in base {}", string, base
      );
    }
  } else {
    throw l3::runtime::RuntimeError(
        "int() takes only primitive values or strings"
    );
  }

  return vm.store_value(l3::runtime::Primitive{value});
}

l3::runtime::Ref builtin_str(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw l3::runtime::RuntimeError("str() takes one arguments");
  }

  std::string result;
  format_args(std::back_inserter(result), args);

  return vm.store_value({std::move(result)});
}

l3::runtime::Ref
builtin_head(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.empty()) {
    throw l3::runtime::RuntimeError("head() takes at least one arguments");
  }

  auto argument = args[0];

  if (const auto &vector_opt = argument->as_vector()) {
    const auto &vector = vector_opt->get();

    if (vector.empty()) {
      throw l3::runtime::RuntimeError("head() takes a non-empty vector");
    }

    auto head = vector.front();
    auto rest = vm.store_value(
        {l3::runtime::Value::vector_type{vector.begin() + 1, vector.end()}}
    );

    return vm.store_value({l3::runtime::Value::vector_type{head, rest}});
  }

  if (const auto &string_opt = argument->as_string()) {
    const auto &string = string_opt->get();

    if (string.empty()) {
      throw l3::runtime::RuntimeError("head() takes a non-empty string");
    }

    auto head = vm.store_new_value({std::string{string.front()}});
    auto rest = vm.store_new_value({string.substr(1)});

    return vm.store_new_value({l3::runtime::Value::vector_type{head, rest}});
  }

  throw l3::runtime::TypeError("head() takes only vector and string values");
}

l3::runtime::Ref
builtin_tail(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.empty()) {
    throw l3::runtime::RuntimeError("tail() takes at least one arguments");
  }

  auto argument = args[0];

  if (const auto &vector_opt = argument->as_vector()) {
    const auto &vector = vector_opt->get();

    if (vector.empty()) {
      throw l3::runtime::RuntimeError("tail() takes a non-empty vector");
    }

    auto tail = vector.back();
    auto rest = vm.store_value(
        {l3::runtime::Value::vector_type{vector.begin(), vector.end() - 1}}
    );

    return vm.store_value({l3::runtime::Value::vector_type{rest, tail}});
  }

  if (const auto &string_opt = argument->as_string()) {
    const auto &string = string_opt->get();

    if (string.empty()) {
      throw l3::runtime::RuntimeError("tail() takes a non-empty string");
    }

    auto tail = vm.store_value({std::string{string.back()}});
    auto rest = vm.store_value({string.substr(0, string.size() - 1)});

    return vm.store_value({l3::runtime::Value::vector_type{rest, tail}});
  }

  throw l3::runtime::TypeError("tail() takes only vector and string values");
}

l3::runtime::Ref builtin_len(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw l3::runtime::RuntimeError("len() takes exactly one arguments");
  }
  return args[0]->visit(
      [&vm](const l3::runtime::ValueContainer auto &container)
          -> l3::runtime::Ref {
        return vm.store_value({l3::runtime::Primitive{
            static_cast<std::int64_t>(container.size())
        }});
      },
      [](const auto &) -> l3::runtime::Ref {
        throw l3::runtime::TypeError("len() does not support {} values");
      }
  );
}

l3::runtime::Ref
builtin_drop(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 2) {
    throw l3::runtime::RuntimeError("drop() takes two arguments");
  }

  auto index_opt =
      args[1]->as_primitive().and_then(&l3::runtime::Primitive::as_integer);
  if (!index_opt) {
    throw l3::runtime::TypeError(
        "drop() takes only an integer as an index argument"
    );
  }
  auto index = *index_opt;

  return vm.store_value(
      args[0]->slice(l3::runtime::Slice{.start = index, .end = std::nullopt})
  );
}

l3::runtime::Ref
builtin_take(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 2) {
    throw l3::runtime::RuntimeError("take() takes two arguments");
  }

  auto index_opt =
      args[1]->as_primitive().and_then(&l3::runtime::Primitive::as_integer);
  if (!index_opt) {
    throw l3::runtime::TypeError(
        "take() takes only an integer as an index argument"
    );
  }
  auto index = *index_opt;

  return vm.store_value(
      args[0]->slice(l3::runtime::Slice{.start = std::nullopt, .end = index})
  );
}

l3::runtime::Ref
builtin_slice(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 3) {
    throw l3::runtime::RuntimeError("slice() takes three arguments");
  }

  auto start_opt =
      args[1]->as_primitive().and_then(&l3::runtime::Primitive::as_integer);
  auto end_opt =
      args[2]->as_primitive().and_then(&l3::runtime::Primitive::as_integer);
  if (!start_opt || !end_opt) {
    throw l3::runtime::TypeError(
        "slice() takes only integers as index arguments"
    );
  }
  auto start = *start_opt;
  auto end = *end_opt;

  return vm.store_value(
      args[0]->slice(l3::runtime::Slice{.start = start, .end = end})
  );
}

l3::runtime::Ref
builtin_random(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  static std::random_device rd;
  static std::mt19937 gen(rd());

  if (args.empty() || args.size() > 2) {
    throw l3::runtime::RuntimeError("random() takes one or two arguments");
  }

  std::optional<std::int64_t> min_opt;
  std::optional<std::int64_t> max_opt;

  if (args.size() == 2) {
    min_opt =
        args[0]->as_primitive().and_then(&l3::runtime::Primitive::as_integer);
    max_opt =
        args[1]->as_primitive().and_then(&l3::runtime::Primitive::as_integer);
  } else {
    min_opt = std::make_optional(std::int64_t{0});
    max_opt =
        args[0]->as_primitive().and_then(&l3::runtime::Primitive::as_integer);
  }

  if (!min_opt || !max_opt) {
    throw l3::runtime::TypeError("random() takes only integers as arguments");
  }

  auto min = *min_opt;
  auto max = *max_opt;

  auto distribution = std::uniform_int_distribution<std::int64_t>{min, max};

  return vm.store_value(
      l3::runtime::Value{l3::runtime::Primitive{distribution(gen)}}
  );
}

l3::runtime::Ref
builtin_sleep(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw l3::runtime::RuntimeError("sleep() takes one argument");
  }
  auto duration_opt =
      args[0]->as_primitive().and_then(&l3::runtime::Primitive::as_integer);
  if (!duration_opt) {
    throw l3::runtime::TypeError(
        "sleep() takes only an integer as an duration argument"
    );
  }
  auto duration = *duration_opt;
  std::this_thread::sleep_for(std::chrono::milliseconds(duration));
  return l3::runtime::Ref{l3::runtime::GCStorage::nil()};
}

l3::runtime::Ref builtin_map(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 2) {
    throw l3::runtime::TypeError("map() takes exactly 2 arguments");
  }

  auto func_opt = args[0]->as_function();
  if (!func_opt) {
    throw l3::runtime::TypeError("map() first argument must be a function");
  }

  const auto list_opt = args[1]->as_vector();
  if (!list_opt) {
    throw l3::runtime::TypeError("map() second argument must be a vector");
  }
  const auto &list = list_opt->get();

  l3::runtime::Value::vector_type result;
  result.reserve(list.size());
  for (const auto &item : list) {
    result.push_back(vm.evaluate(args[0], {item}));
  }

  return vm.store_value(std::move(result));
}

l3::runtime::Ref
builtin_filter(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 2) {
    throw l3::runtime::TypeError("filter() takes exactly 2 arguments");
  }

  auto func_opt = args[0]->as_function();
  if (!func_opt) {
    throw l3::runtime::TypeError("filter() first argument must be a function");
  }

  const auto list_opt = args[1]->as_vector();
  if (!list_opt) {
    throw l3::runtime::TypeError("filter() second argument must be a vector");
  }
  const auto &list = list_opt->get();

  l3::runtime::Value::vector_type result;
  for (const auto &item : list) {
    if (static_cast<vm::BytecodeVM *>(&vm)
            ->evaluate(args[0], {item})
            ->is_truthy()) {
      result.push_back(item);
    }
  }

  return vm.store_value(std::move(result));
}

l3::runtime::Ref
builtin_sum(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw l3::runtime::TypeError("sum() takes exactly 1 argument");
  }

  const auto list_opt = args[0]->as_vector();
  if (!list_opt) {
    throw l3::runtime::TypeError("sum() argument must be a vector");
  }
  const auto &list = list_opt->get();

  if (list.empty()) {
    throw l3::runtime::TypeError("sum() cannot be applied to an empty vector");
  }

  l3::runtime::Ref total = list.front();
  for (auto item : list | std::views::drop(1)) {
    total->add_assign(*item);
  }

  return total;
}

l3::runtime::Ref
builtin_all(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw l3::runtime::TypeError("all() takes exactly 1 argument");
  }

  const auto list_opt = args[0]->as_vector();
  if (!list_opt) {
    throw l3::runtime::TypeError("all() argument must be a vector");
  }
  const auto &list = list_opt->get();

  for (auto item : list) {
    if (item->is_falsy()) {
      return l3::runtime::Ref{l3::runtime::GCStorage::_false()};
    }
  }

  return l3::runtime::Ref{l3::runtime::GCStorage::_true()};
}

l3::runtime::Ref
builtin_any(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw l3::runtime::TypeError("any() takes exactly 1 argument");
  }

  const auto list_opt = args[0]->as_vector();
  if (!list_opt) {
    throw l3::runtime::TypeError("any() argument must be a vector");
  }
  const auto &list = list_opt->get();

  for (auto item : list) {
    if (item->is_truthy()) {
      return l3::runtime::Ref{l3::runtime::GCStorage::_true()};
    }
  }

  return l3::runtime::Ref{l3::runtime::GCStorage::_false()};
}

l3::runtime::Ref
builtin_count(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  if (args.size() != 2) {
    throw l3::runtime::TypeError("count() takes exactly 2 arguments");
  }

  const auto fn_opt = args[0]->as_function();
  if (!fn_opt) {
    throw l3::runtime::TypeError("count() first argument must be a function");
  }

  const auto list_opt = args[1]->as_vector();
  if (!list_opt) {
    throw l3::runtime::TypeError("count() second argument must be a vector");
  }
  const auto &list = list_opt->get();

  std::int64_t count = 0;
  for (const auto &item : list) {
    if (static_cast<vm::BytecodeVM *>(&vm)
            ->evaluate(args[0], {item})
            ->is_truthy()) {
      ++count;
    }
  }

  return vm.store_value(l3::runtime::Primitive{count});
}

l3::runtime::Ref
builtin_identity(l3::vm::BytecodeVM & /*vm*/, l3::runtime::L3Args args) {
  if (args.size() != 1) {
    throw l3::runtime::TypeError("id() takes exactly 1 argument");
  }
  return args[0];
}

l3::runtime::Ref
builtin_range(l3::vm::BytecodeVM &vm, l3::runtime::L3Args args) {
  std::int64_t start = 0;
  std::int64_t end = 0;
  std::int64_t step = 1;

  switch (args.size()) {
  case 1:
    start = 0;
    end = args[0]
              ->as_primitive()
              .and_then(&l3::runtime::Primitive::as_integer)
              .value();
    break;
  case 2:
    start = args[0]
                ->as_primitive()
                .and_then(&l3::runtime::Primitive::as_integer)
                .value();
    end = args[1]
              ->as_primitive()
              .and_then(&l3::runtime::Primitive::as_integer)
              .value();
    break;
  case 3:
    start = args[0]
                ->as_primitive()
                .and_then(&l3::runtime::Primitive::as_integer)
                .value();
    end = args[1]
              ->as_primitive()
              .and_then(&l3::runtime::Primitive::as_integer)
              .value();
    step = args[2]
               ->as_primitive()
               .and_then(&l3::runtime::Primitive::as_integer)
               .value();
    break;
  default:
    throw l3::runtime::TypeError("range() takes 1, 2 or 3 arguments");
  }

  if (step == 0) {
    throw l3::runtime::ValueError("range() step cannot be 0");
  }

  if (step > 0) {
    if (start > end) {
      throw l3::runtime::ValueError("range() start > end");
    }
  } else {
    if (start < end) {
      throw l3::runtime::ValueError("range() start < end with negative step");
    }
  }

  l3::runtime::Value::vector_type result;
  for (std::int64_t i = start; step > 0 ? i < end : i > end; i += step) {
    result.push_back(vm.store_value(l3::runtime::Primitive{i}));
  }
  return vm.store_value(std::move(result));
}

} // namespace

const std::initializer_list<std::pair<
    std::string_view,
    runtime::Ref (*)(l3::vm::BytecodeVM &, runtime::L3Args)>>
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
