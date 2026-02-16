module l3.vm;

import utils;

namespace l3::vm {

ExecutionState::Overlay::Overlay(VM &vm, ExecutionState overlay_state)
    : vm{vm} {
  vm.unused_states.emplace_back(std::move(vm.state));
  vm.state = std::move(overlay_state);
}

ExecutionState::Overlay::~Overlay() {
  vm.state = std::move(vm.unused_states.back());
  vm.unused_states.pop_back();
}

} // namespace l3::vm
