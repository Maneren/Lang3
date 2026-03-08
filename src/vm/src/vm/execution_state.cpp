module l3.vm;

import utils;

namespace l3::vm {

ExecutionState::Overlay::Overlay(VM &vm, ExecutionState overlay_state)
    : vm{vm} {
  vm.state_stack.emplace_back(std::move(overlay_state));
}

ExecutionState::Overlay::~Overlay() { vm.state_stack.pop_back(); }

} // namespace l3::vm
