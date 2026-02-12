export module l3.vm:call_stack;

import std;
import l3.ast;
import l3.location;

export namespace l3::vm {

struct CallStackFrame {
  std::reference_wrapper<const l3::ast::Identifier> function_name;
  std::reference_wrapper<const location::Location> location;
};

using CallStack = std::vector<CallStackFrame>;

class CallStackGuard {
  CallStack
      &call_stack; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

public:
  CallStackGuard(const CallStackGuard &) = delete;
  CallStackGuard(CallStackGuard &&) = delete;
  CallStackGuard &operator=(const CallStackGuard &) = delete;
  CallStackGuard &operator=(CallStackGuard &&) = delete;

  explicit CallStackGuard(CallStack &call_stack, CallStackFrame &&frame)
      : call_stack{call_stack} {
    call_stack.push_back(std::move(frame));
  }
  ~CallStackGuard() { call_stack.pop_back(); }
};

} // namespace l3::vm
