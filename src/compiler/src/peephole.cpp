module l3.compiler;

import std;
import l3.bytecode;
import l3.location;
import l3.runtime;
import utils;

namespace l3::compiler {

namespace {

using namespace l3::bytecode;
using namespace l3::runtime;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool is_primitive_constant(std::size_t idx, const ProgramBytecode &program) {
  return idx < program.constants.size() &&
         program.constants[idx].get_value().is_primitive();
}

Value read_primitive_constant(std::size_t idx, const ProgramBytecode &program) {
  return program.constants[idx].get_value();
}

Value fold_unary(const Instruction &op, const Value &val) {
  if (std::holds_alternative<OpNegate>(op)) {
    return val.negative();
  }
  if (std::holds_alternative<OpNot>(op)) {
    return val.not_op();
  }
  return Nil{};
}

Value fold_binary(const Instruction &op, const Value &lhs, const Value &rhs) {
  if (std::holds_alternative<OpAdd>(op)) {
    return lhs.add(rhs);
  }
  if (std::holds_alternative<OpSubtract>(op)) {
    return lhs.sub(rhs);
  }
  if (std::holds_alternative<OpMultiply>(op)) {
    return lhs.mul(rhs);
  }
  if (std::holds_alternative<OpDivide>(op)) {
    return lhs.div(rhs);
  }
  if (std::holds_alternative<OpModulo>(op)) {
    return lhs.mod(rhs);
  }
  return Nil{};
}

std::size_t add_constant(ProgramBytecode &program, Value &&value) {
  program.constants.emplace_back(std::move(value));
  return program.constants.size() - 1;
}

// ---------------------------------------------------------------------------
// Jump offset adjustment
// ---------------------------------------------------------------------------

void adjust_offset(
    std::size_t &offset, const std::vector<std::size_t> &old_to_new
) {
  if (offset < old_to_new.size()) {
    offset = old_to_new[offset];
  }
}

void adjust_offsets(
    Instruction &inst, const std::vector<std::size_t> &old_to_new
) {
  match::match(
      inst,
      [&](OpJump &jump) { adjust_offset(jump.offset, old_to_new); },
      [&](OpJumpIf &jump) { adjust_offset(jump.offset, old_to_new); },
      [&](OpForLoop &loop) {
        adjust_offset(loop.body_offset, old_to_new);
      },
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

Match try_match(
    std::size_t i,
    const std::vector<Instruction> &code,
    ProgramBytecode &program
) {
  auto &inst = code[i];

  // OpPop{0} → remove (consumed=1, no replacement)
  if (auto *pop = std::get_if<OpPop>(&inst); pop && pop->count == 0) {
    return {.consumed = 1, .replacement = std::nullopt};
  }

  // OpConstant{idx} + OpNegate/OpNot → OpConstant{folded}
  if (auto *cnst = std::get_if<OpConstant>(&inst);
      cnst && i + 1 < code.size()) {
    auto &next = code[i + 1];
    if (is_primitive_constant(cnst->index, program) &&
        (std::holds_alternative<OpNegate>(next) ||
         std::holds_alternative<OpNot>(next))) {
      auto val = read_primitive_constant(cnst->index, program);
      auto folded = fold_unary(next, val);
      if (!folded.is_nil()) {
        auto new_idx = add_constant(program, std::move(folded));
        return {.consumed = 2, .replacement = OpConstant{new_idx}};
      }
    }
  }

  // OpConstant{a} + OpConstant{b} + arithmetic → OpConstant{folded}
  if (auto *cnst = std::get_if<OpConstant>(&inst);
      cnst && i + 2 < code.size()) {
    if (auto *cnst2 = std::get_if<OpConstant>(&code[i + 1]); cnst2) {
      auto &third = code[i + 2];
      if (is_primitive_constant(cnst->index, program) &&
          is_primitive_constant(cnst2->index, program)) {
        auto lhs = read_primitive_constant(cnst->index, program);
        auto rhs = read_primitive_constant(cnst2->index, program);
        auto folded = fold_binary(third, lhs, rhs);
        if (!folded.is_nil()) {
          auto new_idx = add_constant(program, std::move(folded));
          return {.consumed = 3, .replacement = OpConstant{new_idx}};
        }
      }
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
  std::vector<location::Location> new_locations;
  std::vector<std::size_t> old_to_new(chunk.code.size(), -1UZ);
  new_code.reserve(chunk.code.size());
  new_locations.reserve(chunk.code.size());

  for (std::size_t i = 0; i < chunk.code.size();) {
    auto match = try_match(i, chunk.code, program);
    if (match.consumed > 0) {
      if (match.replacement) {
        old_to_new[i] = new_code.size();
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

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void optimize(ProgramBytecode &program) {
  for (auto &chunk : program.chunks) {
    while (run_pass(chunk, program)) {
      // Continue until no more patterns match
    }
  }
}

} // namespace l3::compiler
