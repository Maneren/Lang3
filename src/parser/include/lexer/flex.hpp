#pragma once

#include <string>
#if !defined(yyFlexLexerOnce)
#define yyFlexLexer yy_foo_FlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#endif
#include "lexer/location_t.hh"

namespace foo {
class FooLexer : public yy_foo_FlexLexer {
  std::size_t currentLine = 1;

  std::string *yylval = nullptr;
  location_t *yylloc = nullptr;

  void copyValue(
      const std::size_t leftTrim = 0,
      const std::size_t rightTrim = 0,
      const bool trimCr = false
  );
  void copyLocation() { *yylloc = location_t(currentLine, currentLine); }

public:
  FooLexer(std::istream &in, const bool debug) : yy_foo_FlexLexer(&in) {
    yy_foo_FlexLexer::set_debug(debug);
  }

  int yylex(std::string *const lval, location_t *const lloc);
};

inline void FooLexer::copyValue(
    const std::size_t leftTrim, const std::size_t rightTrim, const bool trimCr
) {
  std::size_t endPos = yyleng - rightTrim;
  if (trimCr && endPos != 0 && yytext[endPos - 1] == '\r')
    --endPos;
  *yylval = std::string(yytext + leftTrim, yytext + endPos);
}
} // namespace foo
