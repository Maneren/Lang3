module l3.bytecode;

namespace l3::bytecode {

void Chunk::write(const Instruction &instruction, std::size_t /*line*/) {
  code.push_back(instruction);
}

} // namespace l3::bytecode
