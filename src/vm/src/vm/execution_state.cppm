export module l3.vm:execution_state;

import :scope;
import :ref_value;
import std;

export namespace l3::vm {

enum class FlowControl : std::uint_fast8_t { Normal, Return, Break, Continue };

class VM;

struct ExecutionState {
  std::shared_ptr<ScopeStack> scopes;
  FlowControl flow_control = FlowControl::Normal;
  std::optional<RefValue> return_value = std::nullopt;
  bool in_function = false;
  bool in_loop = false;

  ExecutionState() = default;
  explicit ExecutionState(std::shared_ptr<ScopeStack> scopes)
      : scopes(std::move(scopes)) {}

  class Overlay {
    VM &vm; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

  public:
    Overlay(const Overlay &) = delete;
    Overlay(Overlay &&) = delete;
    Overlay &operator=(const Overlay &) = delete;
    Overlay &operator=(Overlay &&) = delete;

    explicit Overlay(VM &vm, ExecutionState overlay_state);
    ~Overlay();
  };
};

} // namespace l3::vm

export {
  template <>
  struct std::formatter<l3::vm::FlowControl>
      : utils::static_formatter<l3::vm::FlowControl> {
    static constexpr auto format(auto obj, std::format_context &ctx) {
      switch (obj) {
        using namespace l3::vm;
      case FlowControl::Normal:
        return std::format_to(ctx.out(), "normal");
      case FlowControl::Break:
        return std::format_to(ctx.out(), "break");
      case FlowControl::Continue:
        return std::format_to(ctx.out(), "continue");
      case FlowControl::Return:
        return std::format_to(ctx.out(), "return");
      }
    }
  };
}
