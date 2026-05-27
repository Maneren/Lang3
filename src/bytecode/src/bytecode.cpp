module l3.bytecode;

import l3.location;

namespace l3::bytecode {

void Chunk::write(
    const Instruction &instruction, const location::Location &location
) {
  code.push_back(instruction);
  locations.push_back(location);
}

} // namespace l3::bytecode
