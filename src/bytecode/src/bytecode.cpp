module l3.bytecode;

namespace l3::bytecode {

void Chunk::write(const Instruction &instruction, std::size_t line) {
  code.push_back(instruction);
}

std::size_t Chunk::add_constant(runtime::Value value) {
  constants.push_back(std::move(value));
  return constants.size() - 1;
}

} // namespace l3::bytecode
