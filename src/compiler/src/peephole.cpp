module l3.compiler;

import std;
import l3.bytecode;
import l3.location;
import l3.runtime;
import utils;

namespace {

using namespace l3::bytecode;
using namespace l3::runtime;
using namespace l3::location;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool is_primitive_constant(std::size_t idx, const ProgramBytecode &program) {
  return program.constants[idx].get_value().is_primitive();
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
  const auto cmp = lhs.compare(rhs);
  return match::match<std::optional<HeapData>>(
      op,
      [&](const OpAdd &) { return lhs.add(rhs); },
      [&](const OpSubtract &) { return lhs.sub(rhs); },
      [&](const OpMultiply &) { return lhs.mul(rhs); },
      [&](const OpDivide &) { return lhs.div(rhs); },
      [&](const OpModulo &) { return lhs.mod(rhs); },
      [&](const OpPower &) { return lhs.pow(rhs); },
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

// ---------------------------------------------------------------------------
// Jump offset adjustment
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Pattern matching
// ---------------------------------------------------------------------------

// Result of a pattern match: how many old instructions consumed, and
// what (if any) replacement instruction to emit.
struct Match {
  std::size_t consumed = 0;
  std::optional<Instruction> replacement;
};

// OpPop{0} → remove
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
      [](const auto &) -> Match { return {}; }
  );
}

// OpJump{next_instruction} → remove
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
      [](const auto &) -> Match { return {}; }
  );
}

// OpConstant{idx} + OpNegate/OpNot → OpConstant{folded}
Match match_unary_fold(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  if (i + 1 >= code.size()) {
    return {};
  }
  return match::match<Match>(
      code[i],
      [&](const OpConstant &cnst) -> Match {
        if (!is_primitive_constant(cnst.index, program)) {
          return {};
        }
        return match::match<Match>(
            code[i + 1],
            [&](const OpNegate &) -> Match {
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
            [&](const OpNot &) -> Match {
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
            [](const auto &) -> Match { return {}; }
        );
      },
      [](const auto &) -> Match { return {}; }
  );
}

// OpConstant{x} + OpJumpIf{expected, keep=false, stay=false}
//   x.is_truthy() == expected → OpJump
//   x.is_truthy() != expected → remove both
Match match_const_jumpif(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  if (i + 1 >= code.size()) {
    return {};
  }
  return match::match<Match>(
      code[i],
      [&](const OpConstant &cnst) -> Match {
        if (cnst.index >= program.constants.size()) {
          return {};
        }
        const auto &val = program.constants[cnst.index].get_value();
        if (!val.is_primitive() && !val.is_nil()) {
          return {};
        }
        return match::match<Match>(
            code[i + 1],
            [&](const OpJumpIf &jump) -> Match {
              if (jump.keep_jump || jump.keep_stay) {
                return {};
              }
              if (val.is_truthy() == jump.expected) {
                return {.consumed = 2, .replacement = OpJump{jump.offset}};
              }
              return {.consumed = 2, .replacement = std::nullopt};
            },
            [](const auto &) -> Match { return {}; }
        );
      },
      [](const auto &) -> Match { return {}; }
  );
}

// OpConstant{a} + OpConstant{b} + arithmetic/comparison → OpConstant{folded}
Match match_binary_fold(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  if (i + 2 >= code.size()) {
    return {};
  }
  return match::match<Match>(
      code[i],
      [&](const OpConstant &cnst) -> Match {
        if (!is_primitive_constant(cnst.index, program)) {
          return {};
        }
        return match::match<Match>(
            code[i + 1],
            [&](const OpConstant &cnst2) -> Match {
              if (!is_primitive_constant(cnst2.index, program)) {
                return {};
              }
              const auto &third = code[i + 2];
              auto lhs = read_primitive_constant(cnst.index, program);
              auto rhs = read_primitive_constant(cnst2.index, program);
              if (auto folded = fold_binary(third, lhs, rhs); folded) {
                return {
                    .consumed = 3,
                    .replacement = add_constant(program, std::move(*folded))
                };
              }
              return {};
            },
            [](const auto &) -> Match { return {}; }
        );
      },
      [](const auto &) -> Match { return {}; }
  );
}

// Follow a chain of unconditional jumps to find the final target
std::size_t
follow_jump_chain(std::size_t offset, const std::vector<Instruction> &code) {
  for (;;) {
    auto jumped = match::match<std::optional<std::size_t>>(
        code[offset],
        [offset](const OpJump &tj) -> std::optional<std::size_t> {
          if (tj.offset == offset) {
            return std::nullopt;
          }
          return tj.offset;
        },
        [](const auto &) -> std::optional<std::size_t> { return std::nullopt; }
    );
    if (!jumped) {
      break;
    }
    offset = *jumped;
  }
  return offset;
}

// OpJump{target} → OpJump{target_final} where code[target] jumps to final
// OpJumpIf{target} → OpJumpIf{target_final} where code[target] jumps to final
Match match_jump_chaining(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode & /*unused*/
) {
  return match::match<Match>(code[i], [&](const auto &jump) -> Match {
    using T = std::decay_t<decltype(jump)>;
    if constexpr (std::same_as<T, OpJump> || std::same_as<T, OpJumpIf>) {
      auto target = follow_jump_chain(jump.offset, code);
      if (target != jump.offset) {
        auto replacement = jump;
        replacement.offset = target;
        return {.consumed = 1, .replacement = std::move(replacement)};
      }
    }
    return Match{};
  });
}

Match try_match(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  for (const auto &pattern :
       {match_pop_zero,
        match_zero_jump,
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

// ---------------------------------------------------------------------------
// Single pass over chunk.code: returns true if any pattern matched
// ---------------------------------------------------------------------------

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
      // Continue until no more patterns match
    }
  }
}

} // namespace l3::compiler
