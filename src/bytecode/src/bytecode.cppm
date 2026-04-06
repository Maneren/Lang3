export module l3.bytecode;

import std;
import utils;
import l3.runtime;

export namespace l3::bytecode {

// ----------------------------------------------------------------------------
// Opcodes
// ----------------------------------------------------------------------------
struct OpReturn {
  bool has_value = true;
};
struct OpConstant {
  std::size_t index;
};
struct OpPop {};
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
  std::size_t name_index;
};
struct OpSetGlobal {
  std::size_t name_index;
};

struct OpGetLocal {
  std::size_t index;
};
struct OpSetLocal {
  std::size_t index;
};

struct OpForLoop {
  std::size_t control_index;
  std::size_t limit_index;
  std::size_t body_offset = -1UZ;
  bool inclusive = false;
  std::optional<std::size_t> step_index;
};

struct Upvalue {
  bool is_local;
  std::size_t index;
};
struct OpClosure {
  std::size_t function_index;
  std::vector<Upvalue> upvalues;
};
struct OpGetUpvalue {
  std::size_t index;
};
struct OpSetUpvalue {
  std::size_t index;
};

struct OpJump {
  std::size_t offset = -1UZ;
};
struct OpTest {
  bool pop = true;
  std::size_t offset = -1UZ;
};
struct OpCall {
  std::size_t arg_count;
  bool keep_return_value;
};
struct OpMakeArray {
  std::size_t count;
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
    OpTest,
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

  void write(const Instruction &instruction, std::size_t line);
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
           std::views::enumerate(program.chunks)) {
        out = std::format_to(out, "== Chunk {} ==\n", chunk_id);

        for (const auto &[offset, instruction] :
             std::views::enumerate(chunk.code)) {
          out = std::format_to(out, "{:04d} | ", offset);

          out = match::match(
              instruction,
              [&](const l3::bytecode::OpReturn &op) {
                return std::format_to(
                    out, "{:<16} {:5}\n", "OP_RETURN", op.has_value
                );
              },
              [&](const l3::bytecode::OpConstant &op) {
                return std::format_to(
                    out,
                    "{:<16} {:4d} '{}'\n",
                    "OP_CONSTANT",
                    op.index,
                    program.constants[op.index]
                );
              },
              [&](const l3::bytecode::OpPop &) {
                return std::format_to(out, "OP_POP\n");
              },
              [&](const l3::bytecode::OpDuplicate &op) {
                return std::format_to(
                    out, "{:<16} {:4d}\n", "OP_DUPLICATE", op.index
                );
              },
              [&](const l3::bytecode::OpAdd &) {
                return std::format_to(out, "OP_ADD\n");
              },
              [&](const l3::bytecode::OpSubtract &) {
                return std::format_to(out, "OP_SUBTRACT\n");
              },
              [&](const l3::bytecode::OpMultiply &) {
                return std::format_to(out, "OP_MULTIPLY\n");
              },
              [&](const l3::bytecode::OpDivide &) {
                return std::format_to(out, "OP_DIVIDE\n");
              },
              [&](const l3::bytecode::OpModulo &) {
                return std::format_to(out, "OP_MODULO\n");
              },
              [&](const l3::bytecode::OpNegate &) {
                return std::format_to(out, "OP_NEGATE\n");
              },
              [&](const l3::bytecode::OpEqual &) {
                return std::format_to(out, "OP_EQUAL\n");
              },
              [&](const l3::bytecode::OpNotEqual &) {
                return std::format_to(out, "OP_NOT_EQUAL\n");
              },
              [&](const l3::bytecode::OpGreater &) {
                return std::format_to(out, "OP_GREATER\n");
              },
              [&](const l3::bytecode::OpGreaterEqual &) {
                return std::format_to(out, "OP_GREATER_EQUAL\n");
              },
              [&](const l3::bytecode::OpLess &) {
                return std::format_to(out, "OP_LESS\n");
              },
              [&](const l3::bytecode::OpLessEqual &) {
                return std::format_to(out, "OP_LESS_EQUAL\n");
              },
              [&](const l3::bytecode::OpNot &) {
                return std::format_to(out, "OP_NOT\n");
              },
              [&](const l3::bytecode::OpGetGlobal &op) {
                return std::format_to(
                    out,
                    "{:<16} {:4d} '{}'\n",
                    "OP_GET_GLOBAL",
                    op.name_index,
                    program.constants[op.name_index]
                );
              },
              [&](const l3::bytecode::OpSetGlobal &op) {
                return std::format_to(
                    out,
                    "{:<16} {:4d} '{}'\n",
                    "OP_SET_GLOBAL",
                    op.name_index,
                    program.constants[op.name_index]
                );
              },
              [&](const l3::bytecode::OpGetLocal &op) {
                return std::format_to(
                    out, "{:<16} {:4d}\n", "OP_GET_LOCAL", op.index
                );
              },
              [&](const l3::bytecode::OpSetLocal &op) {
                return std::format_to(
                    out, "{:<16} {:4d}\n", "OP_SET_LOCAL", op.index
                );
              },
              [&](const l3::bytecode::OpForLoop &op) {
                return std::format_to(
                    out,
                    "{:<16} ctrl={:4d} lim={:4d} body={:4d} {} {}\n",
                    "OP_FOR_LOOP",
                    op.control_index,
                    op.limit_index,
                    op.body_offset,
                    op.inclusive ? "LE" : "LT",
                    op.step_index ? std::format("step={:4d}", *op.step_index)
                                  : "step=const1"
                );
              },
              [&](const l3::bytecode::OpJump &op) {
                return std::format_to(
                    out, "{:<16} {:4d}\n", "OP_JUMP", op.offset
                );
              },
              [&](const l3::bytecode::OpTest &op) {
                return std::format_to(
                    out,
                    "{:<16} {:4d} {}\n",
                    "OP_TEST",
                    op.offset,
                    op.pop ? "POP" : "KEEP"
                );
              },
              [&](const l3::bytecode::OpCall &op) {
                return std::format_to(
                    out,
                    "{:<16} {:4d} {:5}\n",
                    "OP_CALL",
                    op.arg_count,
                    op.keep_return_value
                );
              },
              [&](const l3::bytecode::OpMakeArray &op) {
                return std::format_to(
                    out, "{:<16} {:4d}\n", "OP_MAKE_ARRAY", op.count
                );
              },
              [&](const l3::bytecode::OpGetIndex &) {
                return std::format_to(out, "OP_GET_INDEX\n");
              },
              [&](const l3::bytecode::OpSetIndex &) {
                return std::format_to(out, "OP_SET_INDEX\n");
              },
              [&](const l3::bytecode::OpClosure &op) {
                out = std::format_to(
                    out,
                    "{:<16} {:4d} '{}'\n",
                    "OP_CLOSURE",
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
                    out, "{:<16} {:4d}\n", "OP_GET_UPVALUE", op.index
                );
              },
              [&](const l3::bytecode::OpSetUpvalue &op) {
                return std::format_to(
                    out, "{:<16} {:4d}\n", "OP_SET_UPVALUE", op.index
                );
              }
          );
        }
      }

      return out;
    }
  };

} // export
