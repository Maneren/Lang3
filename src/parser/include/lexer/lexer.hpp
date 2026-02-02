#pragma once

#if !defined(yyFlexLexerOnce)
#define yyFlexLexer yy_l3_FlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#endif

namespace l3::lexer {
class L3Lexer;
}

#include <parser.tab.hpp>

namespace l3::lexer {

class L3Lexer : yy_l3_FlexLexer {

public:
  L3Lexer(std::istream &in, bool debug);

  int lex(
      parser::L3Parser::semantic_type *lval,
      parser::L3Parser::location_type *lloc
  );

private:
  parser::L3Parser::semantic_type *yylval = nullptr;
  parser::L3Parser::location_type *yylloc = nullptr;

  void update_location();
};

} // namespace l3::lexer
