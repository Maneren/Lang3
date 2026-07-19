module l3.compiler;

import std;

import utils;

import l3.bytecode;
import l3.location;
import l3.runtime;

namespace {

using namespace l3::bytecode;
using namespace l3::runtime;
using namespace l3::location;

bool is_primitive_constant(std::size_t idx, const ProgramBytecode &program) {
  return program.constants[idx].get_value().is_primitive();
}

bool is_foldable_constant(std::size_t idx, const ProgramBytecode &program) {
  const auto &val = program.constants[idx].get_value();
  return val.is_primitive() || val.is_string();
}

HeapData read_constant(std::size_t idx, const ProgramBytecode &program) {
  return program.constants[idx].get_value();
}

HeapData
read_primitive_constant(std::size_t idx, const ProgramBytecode &program) {
  return program.constants[idx].get_value();
}

std::optional<HeapData> fold_unary(const Instruction &op, const HeapData &val) {
  return match::match<std::optional<HeapData>>(
      op,
      [&](const OpNegate &) { return val.negative(); },
      [&](const OpNot &) { return val.not_op(); },
      [](const auto &) { return std::nullopt; }
  );
}

std::optional<HeapData>
fold_binary(const Instruction &op, const HeapData &lhs, const HeapData &rhs) {
  const auto safe = [&](auto &&fn) -> std::optional<HeapData> {
    try {
      return fn();
    } catch (const UnsupportedOperation &) {
      return std::nullopt;
    }
  };
  const auto cmp = lhs.compare(rhs);
  return match::match<std::optional<HeapData>>(
      op,
      [&](const OpAdd &) { return safe([&] { return lhs.add(rhs); }); },
      [&](const OpSubtract &) { return safe([&] { return lhs.sub(rhs); }); },
      [&](const OpMultiply &) { return safe([&] { return lhs.mul(rhs); }); },
      [&](const OpDivide &) { return safe([&] { return lhs.div(rhs); }); },
      [&](const OpModulo &) { return safe([&] { return lhs.mod(rhs); }); },
      [&](const OpPower &) { return safe([&] { return lhs.pow(rhs); }); },
      [&](const OpEqual &) {
        return HeapData{Primitive{cmp == std::partial_ordering::equivalent}};
      },
      [&](const OpNotEqual &) {
        return HeapData{Primitive{cmp != std::partial_ordering::equivalent}};
      },
      [&](const OpLess &) {
        return HeapData{Primitive{cmp == std::partial_ordering::less}};
      },
      [&](const OpLessEqual &) {
        return HeapData{Primitive{
            cmp == std::partial_ordering::less ||
            cmp == std::partial_ordering::equivalent
        }};
      },
      [&](const OpGreater &) {
        return HeapData{Primitive{cmp == std::partial_ordering::greater}};
      },
      [&](const OpGreaterEqual &) {
        return HeapData{Primitive{
            cmp == std::partial_ordering::greater ||
            cmp == std::partial_ordering::equivalent
        }};
      },
      [](const auto &) { return std::nullopt; }
  );
}

OpConstant add_constant(ProgramBytecode &program, HeapData &&value) {
  program.constants.emplace_back(std::move(value));
  return {program.constants.size() - 1};
}

void adjust_offsets(
    Instruction &inst, const std::vector<std::size_t> &old_to_new
) {
  match::match(
      inst,
      [&](OpJump &jump) { jump.offset = old_to_new[jump.offset]; },
      [&](OpJumpIf &jump) { jump.offset = old_to_new[jump.offset]; },
      [&](OpForLoop &loop) { loop.body_offset = old_to_new[loop.body_offset]; },
      [](auto &) {}
  );
}

// Result of a pattern match: how many old instructions consumed, and
// what (if any) replacement instruction to emit.
struct Match {
  std::size_t consumed = 0;
  std::optional<Instruction> replacement;
};

constexpr auto no_match = [](const auto &...) -> Match { return {}; };

Match match_pop_zero(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode & /*unused*/
) {
  return match::match<Match>(
      code[i],
      [](const OpPop &pop) -> Match {
        if (pop.count != 0) {
          return {};
        }
        return {.consumed = 1, .replacement = std::nullopt};
      },
      no_match
  );
}

Match match_zero_jump(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode & /*unused*/
) {
  return match::match<Match>(
      code[i],
      [i](const OpJump &jump) -> Match {
        if (jump.offset == i + 1) {
          return {.consumed = 1, .replacement = std::nullopt};
        }
        return {};
      },
      [i](const OpJumpIf &jumpif) -> Match {
        if (jumpif.offset == i + 1) {
          return {.consumed = 1, .replacement = std::nullopt};
        }
        return {};
      },
      no_match
  );
}

Match match_merge_pops(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode & /*unused*/
) {
  if (i + 1 >= code.size()) {
    return {};
  }
  return match::match<Match>(
      std::forward_as_tuple(code[i], code[i + 1]),
      [&](const OpPop &first, const OpPop &second) -> Match {
        return Match{
            .consumed = 2,
            .replacement = OpPop{.count = first.count + second.count}
        };
      },
      no_match
  );
}

Match match_unary_fold(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  if (i + 1 >= code.size()) {
    return {};
  }
  return match::match<Match>(
      std::forward_as_tuple(code[i], code[i + 1]),
      [&](const OpConstant &cnst, const OpNegate &) -> Match {
        if (!is_primitive_constant(cnst.index, program)) {
          return {};
        }
        auto val = read_primitive_constant(cnst.index, program);
        auto folded = fold_unary(code[i + 1], val);
        if (!folded) {
          return {};
        }
        return {
            .consumed = 2,
            .replacement = add_constant(program, std::move(*folded))
        };
      },
      [&](const OpConstant &cnst, const OpNot &) -> Match {
        if (!is_primitive_constant(cnst.index, program)) {
          return {};
        }
        auto val = read_primitive_constant(cnst.index, program);
        auto folded = fold_unary(code[i + 1], val);
        if (!folded) {
          return {};
        }
        return {
            .consumed = 2,
            .replacement = add_constant(program, std::move(*folded))
        };
      },
      no_match
  );
}

Match match_const_jumpif(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  if (i + 1 >= code.size()) {
    return {};
  }
  return match::match<Match>(
      std::forward_as_tuple(code[i], code[i + 1]),
      [&](const OpConstant &cnst, const OpJumpIf &jump) -> Match {
        if (cnst.index >= program.constants.size()) {
          return {};
        }
        const auto &val = program.constants[cnst.index].get_value();
        if (!val.is_primitive() && !val.is_nil()) {
          return {};
        }
        if (jump.keep_jump || jump.keep_stay) {
          return {};
        }
        if (val.is_truthy() == jump.expected) {
          return {.consumed = 2, .replacement = OpJump{jump.offset}};
        }
        return {.consumed = 2, .replacement = std::nullopt};
      },
      no_match
  );
}

Match match_binary_fold(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  if (i + 2 >= code.size()) {
    return {};
  }
  return match::match<Match>(
      std::forward_as_tuple(code[i], code[i + 1]),
      [&](const OpConstant &cnst, const OpConstant &cnst2) -> Match {
        if (!is_foldable_constant(cnst.index, program) ||
            !is_foldable_constant(cnst2.index, program)) {
          return {};
        }
        auto lhs = read_constant(cnst.index, program);
        auto rhs = read_constant(cnst2.index, program);
        if (auto folded = fold_binary(code[i + 2], lhs, rhs); folded) {
          return {
              .consumed = 3,
              .replacement = add_constant(program, std::move(*folded))
          };
        }
        return {};
      },
      no_match
  );
}

std::size_t
follow_jump_chain(std::size_t offset, const std::vector<Instruction> &code) {
  for (;;) {
    auto jumped = [&]() -> std::optional<std::size_t> {
      if (const auto *tj = std::get_if<OpJump>(&code[offset])) {
        if (tj->offset == offset) {
          return std::nullopt;
        }
        return tj->offset;
      }
      return std::nullopt;
    }();
    if (!jumped) {
      break;
    }
    offset = *jumped;
  }
  return offset;
}

Match match_jump_chaining(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode & /*unused*/
) {
  return match::match<Match>(
      code[i],
      [&](const OpJump &jump) -> Match {
        auto target = follow_jump_chain(jump.offset, code);
        if (target != jump.offset) {
          auto replacement = jump;
          replacement.offset = target;
          return {.consumed = 1, .replacement = std::move(replacement)};
        }
        return Match{};
      },
      [&](const OpJumpIf &jump) -> Match {
        auto target = follow_jump_chain(jump.offset, code);
        if (target != jump.offset) {
          auto replacement = jump;
          replacement.offset = target;
          return {.consumed = 1, .replacement = std::move(replacement)};
        }
        return Match{};
      },
      no_match
  );
}

Match try_match(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  for (const auto &pattern :
       {match_pop_zero,
        match_zero_jump,
        match_merge_pops,
        match_unary_fold,
        match_const_jumpif,
        match_binary_fold,
        match_jump_chaining}) {
    if (auto m = pattern(i, code, program); m.consumed > 0) {
      return m;
    }
  }
  return {};
}

bool run_pass(Chunk &chunk, ProgramBytecode &program) {
  if (chunk.code.empty()) {
    return false;
  }

  std::vector<Instruction> new_code;
  std::vector<Location> new_locations;
  std::vector<std::size_t> old_to_new(chunk.code.size());
  new_code.reserve(chunk.code.size());
  new_locations.reserve(chunk.code.size());

  for (std::size_t i = 0; i < chunk.code.size();) {
    auto match = try_match(i, chunk.code, program);
    if (match.consumed > 0) {
      for (std::size_t j = i; j < i + match.consumed; ++j) {
        old_to_new[j] = new_code.size();
      }
      if (match.replacement) {
        new_code.push_back(std::move(*match.replacement));
        // Keep the location of the first instruction in the folded group
        if (i < chunk.locations.size()) {
          new_locations.push_back(chunk.locations[i]);
        }
      }
      i += match.consumed;
    } else {
      old_to_new[i] = new_code.size();
      new_code.push_back(chunk.code[i]);
      if (i < chunk.locations.size()) {
        new_locations.push_back(chunk.locations[i]);
      }
      ++i;
    }
  }

  if (new_code.size() == chunk.code.size()) {
    return false;
  }

  for (auto &inst : new_code) {
    adjust_offsets(inst, old_to_new);
  }

  chunk.code = std::move(new_code);
  chunk.locations = std::move(new_locations);
  return true;
}

} // namespace

namespace l3::compiler {

void optimize(bytecode::ProgramBytecode &program) {
  for (auto &chunk : program.chunks) {
    while (run_pass(chunk, program)) {
    }
  }
}

} // namespace l3::compiler
