#include "lexer/flex.hpp"
#include "parser/parser.h"
#include <cstring>
#include <iostream>

int main(int argc, char *argv[]) {
  const bool debug = argc > 1 && std::strcmp(argv[1], "--debug") == 0;
  foo::FooLexer lexer(std::cin, debug);
  foo::FooBisonParser parser(lexer, debug);
  return parser();
}
