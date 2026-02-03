module;

#include <utility>

module l3.vm;

import :variable;

namespace l3::vm {

Variable::Variable(RefValue ref_value, Mutability mutability)
    : ref_value(ref_value), mutability(mutability) {}

} // namespace l3::vm
