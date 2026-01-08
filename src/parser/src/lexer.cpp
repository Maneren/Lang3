#include <parser/lexer.hpp>

namespace l3 {

void L3Lexer::update_location() {
  yylloc->step();

  char *s = yytext;
  for (; *s != '\0'; s++) {
    if (*s == '\n') {
      yylloc->lines();
      yylloc->end.column = 1;
    } else {
      yylloc->columns();
    }
  }
}

L3Lexer::L3Lexer(std::istream &in, bool debug) : yy_l3_FlexLexer(&in) {
  yy_l3_FlexLexer::set_debug(static_cast<int>(debug));
}

} // namespace l3
