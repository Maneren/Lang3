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
    : captures{std::move(captures)}, curried_arguments{nullptr},
      body{function.get_body()} {}
L3Function::L3Function(
    std::shared_ptr<ScopeStack> captures, const ast::NamedFunction &function
)
    : captures{std::move(captures)}, curried_arguments{nullptr},
      body{function.get_body()}, name{function.get_name()} {}

L3Function::L3Function(
    std::shared_ptr<ScopeStack> captures,
    Scope &&curried_arguments,
    std::reference_wrapper<const ast::FunctionBody> body,
    std::optional<Identifier> name
)
    : captures{std::move(captures)},
      curried_arguments{std::make_unique<Scope>(std::move(curried_arguments))},
      body{body}, name{std::move(name)} {};

RefValue L3Function::operator()(VM &vm, L3Args args) {
  const auto &parameters = body.get().get_parameters();

  Scope arguments;
  if (curried_arguments) {
    arguments = curried_arguments->clone(vm);
  } else {
    arguments = Scope{};
  }

  auto needed_arguments = parameters.size() - arguments.size();
  auto remaining_parameters = parameters | std::views::drop(arguments.size());

  if (args.size() > needed_arguments) {
    throw RuntimeError{
        "Function {} expected at most {} arguments, got {}",
        get_name(),
        parameters.size(),
        args.size()
    };
  }

  for (auto [parameter, arg] : std::views::zip(remaining_parameters, args)) {
    arguments.declare_variable(parameter, arg, Mutability::Mutable);
  }

  if (args.size() < needed_arguments) {
    return vm.store_value(
        {L3Function{captures, std::move(arguments), body, name}}
    );
  }

  return vm.evaluate_function_body(captures, std::move(arguments), body.get());
}

L3Function::~L3Function() = default;

const Identifier &L3Function::get_name() const {
  if (name.has_value()) {
    return name.value();
  }
  return anonymous_function_name;
}

Identifier L3Function::anonymous_function_name{std::string_view{"<anonymous>"}};

Function::Function(L3Function &&function) : inner{std::move(function)} {}
Function::Function(BuiltinFunction &&function) : inner{std::move(function)} {}

RefValue Function::operator()(VM &vm, L3Args args) {
  return inner.visit([&vm, &args](auto &func) { return func(vm, args); });
};

BuiltinFunction::BuiltinFunction(Identifier &&name, Body body)
    : name{std::move(name)}, body{body} {}
RefValue BuiltinFunction::operator()(VM &vm, L3Args args) const {
  return body(vm, args);
}
} // namespace l3::vm
