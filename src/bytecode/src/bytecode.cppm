export module l3.bytecode;

import std;
import utils;
import l3.runtime;

export namespace l3::bytecode {

// ----------------------------------------------------------------------------
// Opcodes
// ----------------------------------------------------------------------------
struct OpReturn {};
struct OpConstant {
  std::size_t index;
};
struct OpPop {};

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

struct OpDefineGlobal {
  std::size_t name_index;
};
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
struct OpMutateLocal {
  std::size_t index;
};

struct UpvalueInfo {
  bool is_local;
  std::size_t index;
};
struct OpClosure {
  std::size_t function_index;
  std::vector<UpvalueInfo> upvalues;
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
struct OpJumpIfFalse {
  std::size_t offset = -1UZ;
};
struct OpCall {
  std::size_t arg_count;
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
    OpDefineGlobal,
    OpGetGlobal,
    OpSetGlobal,
    OpGetLocal,
    OpSetLocal,
    OpMutateLocal,
    OpJump,
    OpJumpIfFalse,
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
  std::vector<runtime::Value> constants;

  void write(const Instruction &instruction, std::size_t line);

  std::size_t add_constant(runtime::Value value);
};

// ----------------------------------------------------------------------------
// Printing
// ----------------------------------------------------------------------------
struct NamedChunk {
  const Chunk &chunk;
  std::string_view name;
};

} // namespace l3::bytecode

export {

  template <>
  struct std::formatter<l3::bytecode::NamedChunk>
      : utils::static_formatter<l3::bytecode::NamedChunk> {
    static auto
    format(const l3::bytecode::NamedChunk &nc, std::format_context &ctx) {
      const auto &chunk = nc.chunk;
      auto out = std::format_to(ctx.out(), "== {} ==\n", nc.name);

      for (const auto &[offset, instruction] :
           std::views::enumerate(chunk.code)) {
        out = std::format_to(out, "{:04d} | ", offset);

        out = match::match(
            instruction,
            [&](const l3::bytecode::OpReturn &) {
              return std::format_to(out, "OP_RETURN\n");
            },
            [&](const l3::bytecode::OpConstant &op) {
              return std::format_to(
                  out,
                  "{:<16} {:4d} '{}'\n",
                  "OP_CONSTANT",
                  op.index,
                  chunk.constants[op.index]
              );
            },
            [&](const l3::bytecode::OpPop &) {
              return std::format_to(out, "OP_POP\n");
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
            [&](const l3::bytecode::OpDefineGlobal &op) {
              return std::format_to(
                  out,
                  "{:<16} {:4d} '{}'\n",
                  "OP_DEFINE_GLOBAL",
                  op.name_index,
                  chunk.constants[op.name_index]
              );
            },
            [&](const l3::bytecode::OpGetGlobal &op) {
              return std::format_to(
                  out,
                  "{:<16} {:4d} '{}'\n",
                  "OP_GET_GLOBAL",
                  op.name_index,
                  chunk.constants[op.name_index]
              );
            },
            [&](const l3::bytecode::OpSetGlobal &op) {
              return std::format_to(
                  out,
                  "{:<16} {:4d} '{}'\n",
                  "OP_SET_GLOBAL",
                  op.name_index,
                  chunk.constants[op.name_index]
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
            [&](const l3::bytecode::OpMutateLocal &op) {
              return std::format_to(
                  out, "{:<16} {:4d}\n", "OP_MUTATE_LOCAL", op.index
              );
            },
            [&](const l3::bytecode::OpJump &op) {
              return std::format_to(
                  out, "{:<16} {:4d}\n", "OP_JUMP", op.offset
              );
            },
            [&](const l3::bytecode::OpJumpIfFalse &op) {
              return std::format_to(
                  out, "{:<16} {:4d}\n", "OP_JUMP_IF_FALSE", op.offset
              );
            },
            [&](const l3::bytecode::OpCall &op) {
              return std::format_to(
                  out, "{:<16} {:4d}\n", "OP_CALL", op.arg_count
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
                  chunk.constants[op.function_index]
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

      return out;
    }
  };

} // export
