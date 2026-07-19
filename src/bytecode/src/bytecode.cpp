module l3.bytecode;

import std;

import utils;

import l3.location;
import l3.runtime;

namespace l3::bytecode {

void Chunk::write(Instruction instruction, location::Location location) {
  code.push_back(std::move(instruction));
  locations.push_back(std::move(location));
}

std::string format_instruction(
    const Instruction &inst, const ProgramBytecode &program, std::size_t offset
) {
  auto header = [&] { return std::format("{:04d} | ", offset); };
  return match::match(
      inst,
      [&](const OpReturn &) { return std::format("{}RETURN\n", header()); },
      [&](const OpConstant &op) {
        return std::format(
            "{}{:<10} {:4d} ({})\n",
            header(),
            "CONSTANT",
            op.index,
            program.constants[op.index]
        );
      },
      [&](const OpPop &op) {
        return std::format("{}{:<10} {:4d}\n", header(), "POP", op.count);
      },
      [&](const OpDuplicate &op) {
        return std::format("{}{:<10} {:4d}\n", header(), "DUPLICATE", op.index);
      },
      [&](const OpAdd &) { return std::format("{}ADD\n", header()); },
      [&](const OpSubtract &) { return std::format("{}SUBTRACT\n", header()); },
      [&](const OpMultiply &) { return std::format("{}MULTIPLY\n", header()); },
      [&](const OpDivide &) { return std::format("{}DIVIDE\n", header()); },
      [&](const OpModulo &) { return std::format("{}MODULO\n", header()); },
      [&](const OpPower &) { return std::format("{}POWER\n", header()); },
      [&](const OpNegate &) { return std::format("{}NEGATE\n", header()); },
      [&](const OpEqual &op) {
        auto result = std::format("{}EQUAL", header());
        if (op.keep_rhs)
          result += " keep rhs";
        result += '\n';
        return result;
      },
      [&](const OpNotEqual &op) {
        auto result = std::format("{}NOT_EQUAL", header());
        if (op.keep_rhs)
          result += " keep rhs";
        result += '\n';
        return result;
      },
      [&](const OpGreater &op) {
        auto result = std::format("{}GREATER", header());
        if (op.keep_rhs)
          result += " keep rhs";
        result += '\n';
        return result;
      },
      [&](const OpGreaterEqual &op) {
        auto result = std::format("{}GREATER_EQUAL", header());
        if (op.keep_rhs)
          result += " keep rhs";
        result += '\n';
        return result;
      },
      [&](const OpLess &op) {
        auto result = std::format("{}LESS", header());
        if (op.keep_rhs)
          result += " keep rhs";
        result += '\n';
        return result;
      },
      [&](const OpLessEqual &op) {
        auto result = std::format("{}LESS_EQUAL", header());
        if (op.keep_rhs)
          result += " keep rhs";
        result += '\n';
        return result;
      },
      [&](const OpNot &) { return std::format("{}NOT\n", header()); },
      [&](const OpGetGlobal &op) {
        return std::format(
            "{}{:<10} {:4d} '{}'\n",
            header(),
            "GET_GLOBAL",
            op.name_index,
            program.constants[op.name_index]
        );
      },
      [&](const OpSetGlobal &op) {
        return std::format(
            "{}{:<10} {:4d} '{}'\n",
            header(),
            "SET_GLOBAL",
            op.name_index,
            program.constants[op.name_index]
        );
      },
      [&](const OpGetLocal &op) {
        return std::format("{}{:<10} {:4d}\n", header(), "GET_LOCAL", op.index);
      },
      [&](const OpSetLocal &op) {
        return std::format("{}{:<10} {:4d}\n", header(), "SET_LOCAL", op.index);
      },
      [&](const OpForLoop &op) {
        auto step = op.step_index ? std::format("step={:4d}", *op.step_index)
                                  : "step=const1";
        return std::format(
            "{}{:<10} ctrl={:4d} lim={:4d} body={:4d} {} {}\n",
            header(),
            "FOR_LOOP",
            op.control_index,
            op.limit_index,
            op.body_offset,
            op.inclusive ? "LE" : "LT",
            step
        );
      },
      [&](const OpJump &op) {
        return std::format("{}{:<10} {:4d}\n", header(), "JUMP", op.offset);
      },
      [&](const OpJumpIf &op) {
        auto result = std::format(
            "{}{:<10} {:4d} {}", header(), "JUMP_IF", op.offset, op.expected
        );
        if (op.keep_jump || op.keep_stay) {
          result += " keep after";
        }
        if (op.keep_jump) {
          result += " jump";
        }
        if (op.keep_stay) {
          result += " stay";
        }
        result += '\n';
        return result;
      },
      [&](const OpCall &op) {
        return std::format(
            "{}{:<10} {:4d} {}\n",
            header(),
            "CALL",
            op.arg_count,
            op.keep_return_value
        );
      },
      [&](const OpMakeArray &op) {
        return std::format(
            "{}{:<10} {:4d}\n", header(), "MAKE_ARRAY", op.count
        );
      },
      [&](const OpGetIndex &) {
        return std::format("{}GET_INDEX\n", header());
      },
      [&](const OpSetIndex &) {
        return std::format("{}SET_INDEX\n", header());
      },
      [&](const OpClosure &op) {
        auto result = std::format(
            "{}{:<10} {:4d} '{}'\n",
            header(),
            "CLOSURE",
            op.function_index,
            program.constants[op.function_index]
        );
        for (const auto &upvalue : op.upvalues) {
          result += std::format(
              "{:04d} |                     {} {}\n",
              offset,
              upvalue.is_local ? "local" : "upvalue",
              upvalue.index
          );
        }
        return result;
      },
      [&](const OpGetUpvalue &op) {
        return std::format(
            "{}{:<10} {:4d}\n", header(), "GET_UPVALUE", op.index
        );
      },
      [&](const OpSetUpvalue &op) {
        return std::format(
            "{}{:<10} {:4d}\n", header(), "SET_UPVALUE", op.index
        );
      }
  );
}

} // namespace l3::bytecode
