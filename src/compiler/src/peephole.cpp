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

Value read_primitive_constant(std::size_t idx, const ProgramBytecode &program) {
  return program.constants[idx].get_value();
}

std::optional<Value> fold_unary(const Instruction &op, const Value &val) {
  return match::match<std::optional<Value>>(
      op,
      [&](const OpNegate &) { return val.negative(); },
      [&](const OpNot &) { return val.not_op(); },
      [](const auto &) { return std::nullopt; }
  );
}

std::optional<Value>
fold_binary(const Instruction &op, const Value &lhs, const Value &rhs) {
  const auto cmp = lhs.compare(rhs);
  return match::match<std::optional<Value>>(
      op,
      [&](const OpAdd &) { return lhs.add(rhs); },
      [&](const OpSubtract &) { return lhs.sub(rhs); },
      [&](const OpMultiply &) { return lhs.mul(rhs); },
      [&](const OpDivide &) { return lhs.div(rhs); },
      [&](const OpModulo &) { return lhs.mod(rhs); },
      [&](const OpEqual &) {
        return Value{Primitive{cmp == std::partial_ordering::equivalent}};
      },
      [&](const OpNotEqual &) {
        return Value{Primitive{cmp != std::partial_ordering::equivalent}};
      },
      [&](const OpLess &) {
        return Value{Primitive{cmp == std::partial_ordering::less}};
      },
      [&](const OpLessEqual &) {
        return Value{Primitive{
            cmp == std::partial_ordering::less ||
            cmp == std::partial_ordering::equivalent
        }};
      },
      [&](const OpGreater &) {
        return Value{Primitive{cmp == std::partial_ordering::greater}};
      },
      [&](const OpGreaterEqual &) {
        return Value{Primitive{
            cmp == std::partial_ordering::greater ||
            cmp == std::partial_ordering::equivalent
        }};
      },
      [](const auto &) { return std::nullopt; }
  );
}

OpConstant add_constant(ProgramBytecode &program, Value &&value) {
  program.constants.emplace_back(std::move(value));
  return {program.constants.size() - 1};
}

// ---------------------------------------------------------------------------
// Jump offset adjustment
// ---------------------------------------------------------------------------

void adjust_offset(
    std::size_t &offset, const std::vector<std::size_t> &old_to_new
) {
  offset = old_to_new[offset];
}

void adjust_offsets(
    Instruction &inst, const std::vector<std::size_t> &old_to_new
) {
  match::match(
      inst,
      [&](OpJump &jump) { adjust_offset(jump.offset, old_to_new); },
      [&](OpJumpIf &jump) { adjust_offset(jump.offset, old_to_new); },
      [&](OpForLoop &loop) { adjust_offset(loop.body_offset, old_to_new); },
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

using Pattern = auto (*)(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) -> Match;

// OpPop{0} → remove
Match match_pop_zero(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode & /*unused*/
) {
  const auto *pop = std::get_if<OpPop>(&code[i]);
  if ((pop == nullptr) || pop->count != 0) {
    return {};
  }
  return {.consumed = 1, .replacement = std::nullopt};
}

// OpJump{next_instruction} → remove
Match match_zero_jump(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode & /*unused*/
) {
  const auto *jump = std::get_if<OpJump>(&code[i]);
  if ((jump != nullptr) && jump->offset == i + 1) {
    return {.consumed = 1, .replacement = std::nullopt};
  }

  const auto *jumpif = std::get_if<OpJumpIf>(&code[i]);
  if ((jumpif != nullptr) && jumpif->offset == i + 1) {
    return {.consumed = 1, .replacement = std::nullopt};
  }

  return {};
}

// OpConstant{idx} + OpNegate/OpNot → OpConstant{folded}
Match match_unary_fold(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  const auto *cnst = std::get_if<OpConstant>(&code[i]);
  if ((cnst == nullptr) || i + 1 >= code.size()) {
    return {};
  }
  const auto &next = code[i + 1];
  if (!is_primitive_constant(cnst->index, program) ||
      (!std::holds_alternative<OpNegate>(next) &&
       !std::holds_alternative<OpNot>(next))) {
    return {};
  }
  auto val = read_primitive_constant(cnst->index, program);
  auto folded = fold_unary(next, val);
  if (!folded) {
    return {};
  }
  return {
      .consumed = 2, .replacement = add_constant(program, std::move(*folded))
  };
}

// OpConstant{x} + OpJumpIf{expected, keep=false, stay=false}
//   x.is_truthy() == expected → OpJump
//   x.is_truthy() != expected → remove both
Match match_const_jumpif(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  const auto *cnst = std::get_if<OpConstant>(&code[i]);
  if ((cnst == nullptr) || i + 1 >= code.size() ||
      cnst->index >= program.constants.size()) {
    return {};
  }
  const auto &val = program.constants[cnst->index].get_value();
  if (!val.is_primitive() && !val.is_nil()) {
    return {};
  }
  const auto *jump = std::get_if<OpJumpIf>(&code[i + 1]);
  if ((jump == nullptr) || jump->keep_jump || jump->keep_stay) {
    return {};
  }
  if (val.is_truthy() == jump->expected) {
    return {.consumed = 2, .replacement = OpJump{jump->offset}};
  }
  return {.consumed = 2, .replacement = std::nullopt};
}

// OpConstant{a} + OpConstant{b} + arithmetic/comparison → OpConstant{folded}
Match match_binary_fold(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  const auto *cnst = std::get_if<OpConstant>(&code[i]);
  if ((cnst == nullptr) || !is_primitive_constant(cnst->index, program) ||
      i + 2 >= code.size()) {
    return {};
  }
  const auto *cnst2 = std::get_if<OpConstant>(&code[i + 1]);
  if ((cnst2 == nullptr) || !is_primitive_constant(cnst2->index, program)) {
    return {};
  }
  const auto &third = code[i + 2];
  auto lhs = read_primitive_constant(cnst->index, program);
  auto rhs = read_primitive_constant(cnst2->index, program);

  if (auto folded = fold_binary(third, lhs, rhs); folded) {
    return {
        .consumed = 3, .replacement = add_constant(program, std::move(*folded))
    };
  }
  return {};
}

// OpJump{target} → OpJump{target2} where code[target] is OpJump{target2}
// OpJumpIf{target} → OpJumpIf{target2} where code[target] is OpJump{target2}
template <typename JumpT>
Match try_jump_chain(const JumpT &jump, const std::vector<Instruction> &code) {
  if (const auto *tj = std::get_if<OpJump>(&code[jump.offset])) {
    if (tj->offset != jump.offset) {
      auto replacement = jump;
      replacement.offset = tj->offset;
      return {.consumed = 1, .replacement = replacement};
    }
  }
  return {};
}

Match match_jump_chaining(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode & /*unused*/
) {
  const auto &inst = code[i];
  if (const auto *jump = std::get_if<OpJump>(&inst)) {
    return try_jump_chain(*jump, code);
  }
  if (const auto *jump = std::get_if<OpJumpIf>(&inst)) {
    return try_jump_chain(*jump, code);
  }
  return {};
}

constexpr std::array<Pattern, 6> patterns{
    match_pop_zero,
    match_zero_jump,
    match_unary_fold,
    match_const_jumpif,
    match_binary_fold,
    match_jump_chaining,
};

Match try_match(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  for (auto pattern : patterns) {
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
  std::vector<std::size_t> old_to_new(chunk.code.size(), -1UZ);
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
