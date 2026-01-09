#include "vm/function.hpp"
#include "vm/error.hpp"
#include "vm/scope.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"
#include <ast/nodes/function.hpp>
#include <ast/nodes/identifier.hpp>
#include <ast/printing.hpp>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace l3::vm {

L3Function::L3Function(
    const std::vector<std::shared_ptr<Scope>> &active_scopes,
    const ast::AnonymousFunction &function
)
    : capture_scopes{active_scopes}, body{function.get_body()} {}
L3Function::L3Function(
    const std::vector<std::shared_ptr<Scope>> &active_scopes,
    const ast::NamedFunction &function
)
    : capture_scopes{active_scopes}, body{function.get_body()},
      name{function.get_name()} {}

L3Function::L3Function(
    std::vector<std::shared_ptr<Scope>> &&active_scopes,
    ast::FunctionBody body,
    std::optional<ast::Identifier> name
)
    : capture_scopes{std::move(active_scopes)}, body{std::move(body)},
      name{std::move(name)} {};

CowValue L3Function::operator()(VM &vm, std::span<const CowValue> args) {
  const auto &parameters = body.get_parameters();
  if (args.size() > parameters.size()) {
    throw RuntimeError{
        "Function {} expected at most {} arguments, got {}",
        get_name(),
        parameters.size(),
        args.size()
    };
  }

  auto arguments = Scope{};
  for (const auto &[parameter, arg] : std::views::zip(parameters, args)) {
    arguments.declare_variable(parameter) = arg.to_value<Value>();
  }

  if (args.size() < parameters.size()) {
    auto new_scopes = capture_scopes;
    new_scopes.push_back(std::make_shared<Scope>(std::move(arguments)));

    auto new_body = ast::FunctionBody{
        parameters | std::views::drop(args.size()) |
            std::ranges::to<ast::NameList>(),
        body.get_block_ptr()
    };
    return CowValue{
        Value{L3Function{std::move(new_scopes), std::move(new_body), name}}
    };
  }

  return vm.evaluate_function_body(capture_scopes, std::move(arguments), body);
}

L3Function::~L3Function() = default;

const ast::Identifier &L3Function::get_name() const {
  if (name.has_value()) {
    return name.value();
  }
  return anonymous_function_name;
}

ast::Identifier L3Function::anonymous_function_name{
    std::string_view{"<anonymous>"}
};

Function::Function(L3Function &&function) : inner{std::move(function)} {}
Function::Function(BuiltinFunction &&function) : inner{std::move(function)} {}

CowValue Function::operator()(VM &vm, std::span<const CowValue> args) {
  return inner.visit([&vm, &args](auto &func) { return func(vm, args); });
};

BuiltinFunction::BuiltinFunction(ast::Identifier &&name, Body &&body)
    : name{std::move(name)}, body{std::move(body)} {}
CowValue
BuiltinFunction::operator()(VM &vm, std::span<const CowValue> args) const {
  return body(vm, args);
}
} // namespace l3::vm
