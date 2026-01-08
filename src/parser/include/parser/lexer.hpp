#pragma once

#if !defined(yyFlexLexerOnce)
#define yyFlexLexer yy_l3_FlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#endif

namespace l3 {
class L3Lexer;
}

#include <parser/parser.tab.h>

namespace l3 {

class L3Lexer : yy_l3_FlexLexer {

public:
  L3Lexer(std::istream &in, bool debug);

  int lex(L3Parser::semantic_type *lval, L3Parser::location_type *lloc);

private:
  L3Parser::semantic_type *yylval = nullptr;
  L3Parser::location_type *yylloc = nullptr;

  void update_location();
};

} // namespace l3
