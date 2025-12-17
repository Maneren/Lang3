#pragma once

#if !defined(yyFlexLexerOnce)
#define yyFlexLexer yy_l3_FlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#endif

namespace l3 {
class L3Lexer;
}

#include "parser.tab.h"

namespace l3 {

class L3Lexer : public yy_l3_FlexLexer {
  L3Parser::semantic_type *yylval = nullptr;
  location *yylloc = nullptr;

public:
  L3Lexer(std::istream &in, const bool debug) : yy_l3_FlexLexer(&in) {
    yy_l3_FlexLexer::set_debug(static_cast<int>(debug));
  }

  int lex(L3Parser::semantic_type *lval, L3Parser::location_type *lloc);
};

} // namespace l3
