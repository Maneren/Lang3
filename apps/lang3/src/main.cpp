#include "lexer/lexer.hpp"
#include "parser.tab.h"
#include <cstring>
#include <print>

int main(int argc, char *argv[]) {
  const bool debug = argc > 1 && std::strcmp(argv[1], "--debug") == 0;
  l3::L3Lexer lexer(std::cin, debug);
  l3::L3Parser parser(lexer, debug);
  const auto result = parser();

  std::println("result: {}", result);
  return 0;
}
