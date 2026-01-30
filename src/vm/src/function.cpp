#include "vm/function.hpp"
#include "vm/error.hpp"
#include "vm/scope.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"
#include <memory>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

namespace l3::vm {

L3Function::L3Function(
    const ScopeStack &captures, const ast::AnonymousFunction &function
)
    : captures{captures}, curried_arguments{std::make_shared<Scope>()},
      body{std::make_shared<ast::FunctionBody>(function.get_body())} {}
L3Function::L3Function(
    const ScopeStack &captures, const ast::NamedFunction &function
)
    : captures{captures}, curried_arguments{std::make_shared<Scope>()},
      body{std::make_shared<ast::FunctionBody>(function.get_body())},
      name{function.get_name()} {}

L3Function::L3Function(
    ScopeStack captures,
    Scope &&curried_arguments,
    std::shared_ptr<ast::FunctionBody> body,
    std::optional<Identifier> name
)
    : captures{std::move(captures)},
      curried_arguments{std::make_shared<Scope>(std::move(curried_arguments))},
      body{std::move(body)}, name{std::move(name)} {};

RefValue L3Function::operator()(VM &vm, L3Args args) {
  const auto &parameters = body->get_parameters();
  auto needed_arguments = parameters.size() - curried_arguments->size();
  auto remaining_parameters =
      parameters | std::views::drop(curried_arguments->size());

  if (args.size() > needed_arguments) {
    throw RuntimeError{
        "Function {} expected at most {} arguments, got {}",
        get_name(),
        parameters.size(),
        args.size()
    };
  }

  if (args.size() < needed_arguments) {
    auto new_curried = curried_arguments->clone(vm);
    for (auto [parameter, arg] : std::views::zip(parameters, args)) {
      new_curried.declare_variable(parameter, arg, Mutability::Mutable);
    }
    return vm.store_value(
        {L3Function{captures, std::move(new_curried), body, name}}
    );
  }

  auto arguments = curried_arguments->clone(vm);

  for (auto [parameter, arg] : std::views::zip(remaining_parameters, args)) {
    arguments.declare_variable(parameter, arg, Mutability::Mutable);
  }

  return vm.evaluate_function_body(captures, std::move(arguments), *body);
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
