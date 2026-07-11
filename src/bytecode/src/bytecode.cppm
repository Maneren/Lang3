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
struct OpPower {};
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
    OpPower,
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
  std::vector<runtime::HeapCell> constants;
};

[[nodiscard]] std::string format_instruction(
    const Instruction &inst, const ProgramBytecode &program, std::size_t offset
);

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
          out = std::format_to(
              out, "{}", format_instruction(instruction, program, offset)
          );
        }
      }

      return out;
    }
  };

} // export
