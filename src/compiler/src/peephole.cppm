export module l3.compiler:peephole;

import std;
import l3.bytecode;

export namespace l3::compiler {

void optimize(bytecode::ProgramBytecode &program);

}
