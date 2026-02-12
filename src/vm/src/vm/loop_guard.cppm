export module l3.vm:loop_guard;

import :execution_state;

export namespace l3::vm {

class LoopGuard {
  ExecutionState
      &state; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  bool previous_in_loop;

public:
  LoopGuard(const LoopGuard &) = delete;
  LoopGuard(LoopGuard &&) = delete;
  LoopGuard &operator=(const LoopGuard &) = delete;
  LoopGuard &operator=(LoopGuard &&) = delete;

  explicit LoopGuard(ExecutionState &state)
      : state{state}, previous_in_loop{state.in_loop} {
    state.in_loop = true;
  }
  ~LoopGuard() { state.in_loop = previous_in_loop; }
};

} // namespace l3::vm
