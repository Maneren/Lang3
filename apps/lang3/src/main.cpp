#include "ast/printing.hpp"
#include "cli/cli.hpp"
#include "lexer/lexer.hpp"
#include "parser.tab.h"
#include "vm/vm.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <print>

int main(int argc, char *argv[]) {
  cli::Parser cli_parser = cli::Parser{}.flag("d", "debug");
  const auto args = cli_parser.parse(argc, argv);

  if (!args) {
    std::println(std::cerr, "{}", args.error().what());
    return EXIT_FAILURE;
  }

  const auto &positional = args->positional();

  const bool debug = args->has_flag("debug");

  std::istream *input = &std::cin;
  std::string filename = "<stdin>";

  std::ifstream input_file;

  if (positional.size() == 1 && positional[0] != "-") {
    input_file = std::ifstream{positional[0]};
    input = &input_file;
    filename = positional[0];
  }

  l3::L3Lexer lexer(*input, debug);

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

  return EXIT_SUCCESS;
}
