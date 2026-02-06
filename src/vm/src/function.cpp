module;

#include <memory>
#include <optional>
#include <ranges>
#include <utility>

module l3.vm;

import l3.ast;
import :variable;

namespace l3::vm {

L3Function::L3Function(
    std::shared_ptr<ScopeStack> captures, const ast::AnonymousFunction &function
)
    : captures{std::move(captures)}, body{function.get_body()} {}
L3Function::L3Function(
    std::shared_ptr<ScopeStack> captures, const ast::NamedFunction &function
)
    : captures{std::move(captures)}, body{function.get_body()},
      name{function.get_name()} {}

L3Function::L3Function(
    std::shared_ptr<ScopeStack> captures,
    Scope &&curried,
    std::reference_wrapper<const ast::FunctionBody> body,
    std::optional<Identifier> name
)
    : captures{std::move(captures)}, curried{std::move(curried)}, body{body},
      name{std::move(name)} {};

L3Function::~L3Function() = default;

Identifier L3Function::anonymous_function_name{std::string_view{"<anonymous>"}};

Function::Function(L3Function &&function) : inner{std::move(function)} {}
Function::Function(BuiltinFunction &&function) : inner{std::move(function)} {}

BuiltinFunction::BuiltinFunction(Identifier &&name, Body body)
    : name{std::move(name)}, body{body} {}

} // namespace l3::vm
