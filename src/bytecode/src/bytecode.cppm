export module l3.bytecode;

import std;
import utils;
import l3.location;
import l3.runtime;

export namespace l3::bytecode {

// ----------------------------------------------------------------------------
// Opcodes
// ----------------------------------------------------------------------------
struct OpReturn {};
struct OpConstant {
  std::size_t index = -1UZ;
};
struct OpPop {
  std::size_t count = 1;
};
struct OpDuplicate {
  std::size_t index = 0;
};

struct OpAdd {};
struct OpSubtract {};
struct OpMultiply {};
struct OpDivide {};
struct OpModulo {};
struct OpNegate {};

struct OpEqual {};
struct OpNotEqual {};
struct OpGreater {};
struct OpGreaterEqual {};
struct OpLess {};
struct OpLessEqual {};
struct OpNot {};

struct OpGetGlobal {
  std::size_t name_index = -1UZ;
};
struct OpSetGlobal {
  std::size_t name_index = -1UZ;
};

struct OpGetLocal {
  std::size_t index = -1UZ;
};
struct OpSetLocal {
  std::size_t index = -1UZ;
};

struct OpForLoop {
  std::size_t control_index = -1UZ;
  std::size_t limit_index = -1UZ;
  std::size_t body_offset = -1UZ;
  bool inclusive = false;
  std::optional<std::size_t> step_index;
};

struct Upvalue {
  bool is_local = true;
  std::size_t index = -1UZ;
};
struct OpClosure {
  std::size_t function_index = -1UZ;
  std::vector<Upvalue> upvalues;
};
struct OpGetUpvalue {
  std::size_t index = -1UZ;
};
struct OpSetUpvalue {
  std::size_t index = -1UZ;
};

struct OpJump {
  std::size_t offset = -1UZ;
};
struct OpJumpIf {
  std::size_t offset = -1UZ;
  bool expected = false;
  bool keep_stay = false;
  bool keep_jump = false;
};
struct OpCall {
  std::size_t arg_count = -1UZ;
  bool keep_return_value = true;
};
struct OpMakeArray {
  std::size_t count = -1UZ;
};
struct OpGetIndex {};
struct OpSetIndex {};

// ----------------------------------------------------------------------------
// Instruction
// ----------------------------------------------------------------------------
using Instruction = std::variant<
    OpReturn,
    OpConstant,
    OpPop,
    OpDuplicate,
    OpAdd,
    OpSubtract,
    OpMultiply,
    OpDivide,
    OpModulo,
    OpNegate,
    OpEqual,
    OpNotEqual,
    OpGreater,
    OpGreaterEqual,
    OpLess,
    OpLessEqual,
    OpNot,
    OpGetGlobal,
    OpSetGlobal,
    OpGetLocal,
    OpSetLocal,
    OpForLoop,
    OpJump,
    OpJumpIf,
    OpCall,
    OpMakeArray,
    OpGetIndex,
    OpSetIndex,
    OpClosure,
    OpGetUpvalue,
    OpSetUpvalue>;

// ----------------------------------------------------------------------------
// Chunk
// ----------------------------------------------------------------------------
class Chunk {
public:
  std::vector<Instruction> code;
  std::vector<location::Location> locations;

  void
  write(const Instruction &instruction, const location::Location &location);
};

struct ProgramBytecode {
  std::vector<Chunk> chunks;
  std::vector<runtime::GCValue> constants;
};

} // namespace l3::bytecode

export {

  template <>
  struct std::formatter<l3::bytecode::ProgramBytecode>
      : utils::static_formatter<l3::bytecode::ProgramBytecode> {
    static auto format(
        const l3::bytecode::ProgramBytecode &program, std::format_context &ctx
    ) {
      auto out = ctx.out();

      for (const auto &[chunk_id, chunk] :
           utils::ranges::enumerate(program.chunks)) {
        out = std::format_to(out, "== Chunk {} ==\n", chunk_id);

        for (const auto &[offset, instruction] :
             utils::ranges::enumerate(chunk.code)) {
          out = std::format_to(out, "{:04d} | ", offset);

          out = match::match(
              instruction,
              [&](const l3::bytecode::OpReturn &) {
                return std::format_to(out, "RETURN\n");
              },
              [&](const l3::bytecode::OpConstant &op) {
                return std::format_to(
                    out,
                    "{:<10} {:4d} ({})\n",
                    "CONSTANT",
                    op.index,
                    program.constants[op.index]
                );
              },
              [&](const l3::bytecode::OpPop &op) {
                return std::format_to(out, "{:<10} {:4d}\n", "POP", op.count);
              },
              [&](const l3::bytecode::OpDuplicate &op) {
                return std::format_to(
                    out, "{:<10} {:4d}\n", "DUPLICATE", op.index
                );
              },
              [&](const l3::bytecode::OpAdd &) {
                return std::format_to(out, "ADD\n");
              },
              [&](const l3::bytecode::OpSubtract &) {
                return std::format_to(out, "SUBTRACT\n");
              },
              [&](const l3::bytecode::OpMultiply &) {
                return std::format_to(out, "MULTIPLY\n");
              },
              [&](const l3::bytecode::OpDivide &) {
                return std::format_to(out, "DIVIDE\n");
              },
              [&](const l3::bytecode::OpModulo &) {
                return std::format_to(out, "MODULO\n");
              },
              [&](const l3::bytecode::OpNegate &) {
                return std::format_to(out, "NEGATE\n");
              },
              [&](const l3::bytecode::OpEqual &) {
                return std::format_to(out, "EQUAL\n");
              },
              [&](const l3::bytecode::OpNotEqual &) {
                return std::format_to(out, "NOT_EQUAL\n");
              },
              [&](const l3::bytecode::OpGreater &) {
                return std::format_to(out, "GREATER\n");
              },
              [&](const l3::bytecode::OpGreaterEqual &) {
                return std::format_to(out, "GREATER_EQUAL\n");
              },
              [&](const l3::bytecode::OpLess &) {
                return std::format_to(out, "LESS\n");
              },
              [&](const l3::bytecode::OpLessEqual &) {
                return std::format_to(out, "LESS_EQUAL\n");
              },
              [&](const l3::bytecode::OpNot &) {
                return std::format_to(out, "NOT\n");
              },
              [&](const l3::bytecode::OpGetGlobal &op) {
                return std::format_to(
                    out,
                    "{:<10} {:4d} '{}'\n",
                    "GET_GLOBAL",
                    op.name_index,
                    program.constants[op.name_index]
                );
              },
              [&](const l3::bytecode::OpSetGlobal &op) {
                return std::format_to(
                    out,
                    "{:<10} {:4d} '{}'\n",
                    "SET_GLOBAL",
                    op.name_index,
                    program.constants[op.name_index]
                );
              },
              [&](const l3::bytecode::OpGetLocal &op) {
                return std::format_to(
                    out, "{:<10} {:4d}\n", "GET_LOCAL", op.index
                );
              },
              [&](const l3::bytecode::OpSetLocal &op) {
                return std::format_to(
                    out, "{:<10} {:4d}\n", "SET_LOCAL", op.index
                );
              },
              [&](const l3::bytecode::OpForLoop &op) {
                return std::format_to(
                    out,
                    "{:<10} ctrl={:4d} lim={:4d} body={:4d} {} {}\n",
                    "FOR_LOOP",
                    op.control_index,
                    op.limit_index,
                    op.body_offset,
                    op.inclusive ? "LE" : "LT",
                    op.step_index ? std::format("step={:4d}", *op.step_index)
                                  : "step=const1"
                );
              },
              [&](const l3::bytecode::OpJump &op) {
                return std::format_to(out, "{:<10} {:4d}\n", "JUMP", op.offset);
              },
              [&](const l3::bytecode::OpJumpIf &op) {
                std::format_to(
                    out, "{:<10} {:4d} {}", "JUMP_IF", op.offset, op.expected
                );

                if (op.keep_jump || op.keep_stay) {
                  out = std::format_to(out, " keep after");
                }

                if (op.keep_jump) {
                  out = std::format_to(out, " jump");
                }
                if (op.keep_stay) {
                  out = std::format_to(out, " stay");
                }

                return std::format_to(out, "\n");
              },
              [&](const l3::bytecode::OpCall &op) {
                return std::format_to(
                    out,
                    "{:<10} {:4d} {}\n",
                    "CALL",
                    op.arg_count,
                    op.keep_return_value
                );
              },
              [&](const l3::bytecode::OpMakeArray &op) {
                return std::format_to(
                    out, "{:<10} {:4d}\n", "MAKE_ARRAY", op.count
                );
              },
              [&](const l3::bytecode::OpGetIndex &) {
                return std::format_to(out, "GET_INDEX\n");
              },
              [&](const l3::bytecode::OpSetIndex &) {
                return std::format_to(out, "SET_INDEX\n");
              },
              [&](const l3::bytecode::OpClosure &op) {
                out = std::format_to(
                    out,
                    "{:<10} {:4d} '{}'\n",
                    "CLOSURE",
                    op.function_index,
                    program.constants[op.function_index]
                );
                for (const auto &upvalue : op.upvalues) {
                  out = std::format_to(
                      out,
                      "{:04d} |                     {} {}\n",
                      offset,
                      upvalue.is_local ? "local" : "upvalue",
                      upvalue.index
                  );
                }
                return out;
              },
              [&](const l3::bytecode::OpGetUpvalue &op) {
                return std::format_to(
                    out, "{:<10} {:4d}\n", "GET_UPVALUE", op.index
                );
              },
              [&](const l3::bytecode::OpSetUpvalue &op) {
                return std::format_to(
                    out, "{:<10} {:4d}\n", "SET_UPVALUE", op.index
                );
              }
          );
        }
      }

      return out;
    }
  };

} // export
