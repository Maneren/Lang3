module;

#include <memory>
#include <optional>
#include <unordered_map>
#include <utils/accessor.h>
#include <vector>

export module l3.vm:scope;

import l3.ast;
import utils;
import :identifier;
import :mutability;

export namespace l3::vm {

class VM;
class GCValue;
class Variable;
class RefValue;

class Scope {
public:
  using VariableMap = std::unordered_map<Identifier, Variable>;
  using BuiltinsMap = std::unordered_map<Identifier, GCValue>;

private:
  static BuiltinsMap builtins;
  VariableMap variables;

public:
  Scope() = default;
  Scope(VariableMap &&variables);

  Variable &declare_variable(
      const Identifier &id, RefValue ref_value, Mutability mutability
  );

  utils::optional_cref<Variable> get_variable(const Identifier &id) const;
  utils::optional_ref<Variable> get_variable_mut(const Identifier &id);

  bool has_variable(const Identifier &id) const;
  size_t size() const;

  DEFINE_ACCESSOR_X(variables);

  void mark_gc();

  static std::optional<RefValue> get_builtin(const Identifier &id);

  [[nodiscard]] Scope clone(VM &vm) const;
};

class ScopeStack {
  std::vector<std::shared_ptr<Scope>> scopes;

public:
  [[nodiscard]] const Scope &top() const;
  [[nodiscard]] Scope &top();

  [[nodiscard]] ScopeStack clone(VM &vm) const;

  [[nodiscard]]
  std::optional<RefValue> read_variable(const Identifier &id) const;
  [[nodiscard]]
  utils::optional_ref<RefValue> read_variable_mut(const Identifier &id);

  void pop_back();
  void push_back(std::shared_ptr<Scope> &&scope);
  [[nodiscard]] size_t size() const;

  DEFINE_ACCESSOR_X(scopes);

  class FrameGuard {
    ScopeStack &scope_stack;

  public:
    explicit FrameGuard(ScopeStack &scope_stack);
    explicit FrameGuard(ScopeStack &scope_stack, Scope &&scope);

    FrameGuard(const FrameGuard &) = delete;
    FrameGuard(FrameGuard &&) = delete;
    FrameGuard &operator=(const FrameGuard &) = delete;
    FrameGuard &operator=(FrameGuard &&) = delete;

    ~FrameGuard();
  };

  [[nodiscard]] FrameGuard with_frame();
  [[nodiscard]] FrameGuard with_frame(Scope &&scope);
};

} // namespace l3::vm
