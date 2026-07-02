export module l3.compiler:peephole;

import std;
import l3.bytecode;

using namespace l3::bytecode;

namespace l3::compiler {

void optimize(ProgramBytecode &program);

}
