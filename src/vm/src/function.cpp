#include "vm/function.hpp"
#include "vm/scope.hpp"
#include "vm/vm.hpp"

#include <ranges>

namespace l3::vm {

CowValue L3Function::operator()(VM &vm, std::span<const CowValue> args) {
  const auto &parameters = body.get_parameters();
  if (args.size() != parameters.size()) {
    throw RuntimeError{
        "Function {} expects {} arguments, got {}",
        get_name(),
        parameters.size(),
        args.size()
    };
  }

  auto arguments = Scope{};
  for (const auto &[parameter, arg] : std::views::zip(parameters, args)) {
    arguments.declare_variable(parameter) = arg.to_value<Value>();
  }

  return vm.evaluate_function_body(capture_scopes, std::move(arguments), body);
}

L3Function::~L3Function() = default;

Function::Function(L3Function &&function) : inner{std::move(function)} {}
Function::Function(BuiltinFunction &&function) : inner{std::move(function)} {}

const ast::Identifier &L3Function::get_name() const {
  if (name.has_value()) {
    return name.value();
  }
  return anonymous_function_name;
}

ast::Identifier L3Function::anonymous_function_name{
    std::string_view{"<anonymous>"}
};

} // namespace l3::vm
