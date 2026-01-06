#include "ast/printing.hpp"
#include "lexer/lexer.hpp"
#include "parser.tab.h"
#include "vm/vm.hpp"
#include <cstring>
#include <print>

int main(int argc, char *argv[]) {
  const auto args = std::span{argv, static_cast<size_t>(argc)};

  const bool debug = argc > 1 && std::strcmp(args[1], "--debug") == 0;

  std::string filename = "<stdin>";

  l3::L3Lexer lexer(std::cin, debug);

  auto program = l3::ast::Program{};

  l3::L3Parser parser(lexer, filename, debug, program);
  const auto result = parser.parse();

  if (result != 0) {
    std::println(std::cerr, "Syntax error, returning {}", result);
    return result;
  }

  if (debug) {
    std::print(std::cerr, "{}", program);
  }

  l3::vm::VM vm;

  vm.execute(program);

  return 0;
}
