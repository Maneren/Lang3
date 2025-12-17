#include "lexer/lexer.hpp"
#include "parser.tab.h"
#include <cstring>

int main(int argc, char *argv[]) {
  const auto args = std::span{argv, static_cast<size_t>(argc)};

  const bool debug = argc > 1 && std::strcmp(args[1], "--debug") == 0;

  l3::L3Lexer lexer(std::cin, debug);

  auto program = l3::ast::Program{};

  l3::L3Parser parser(lexer, debug, program);
  const auto result = parser();

  if (result != 0) {
    return result;
  }

  std::ostreambuf_iterator<char> out_iterator(std::cout);
  program.print(out_iterator, 0);

  return 0;
}
