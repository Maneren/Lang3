%{
    #include "lexer/flex.hpp"
    #include "parser/parser.h"

    using namespace foo;

    #undef  YY_DECL
    #define YY_DECL int FooLexer::yylex(std::string *const lval, location_t *const lloc)

    #define YY_USER_INIT yylval = lval; yylloc = lloc;

    #define YY_USER_ACTION copyLocation();
%}

%option c++ noyywrap debug

%option yyclass="FooLexer"
%option prefix="yy_foo_"

%%

%{
    using Token = FooBisonParser::token;
%}

\n { ++currentLine; }
[[:space:]] ;
Hello { return Token::HELLO; }
[[:alpha:]]+ { copyValue(); return Token::WORLD; }
. { return yytext[0]; }
