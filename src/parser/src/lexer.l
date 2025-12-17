%{
    #include "lexer/lexer.hpp"
    #include "parser.tab.h"

    using namespace l3;

    #undef  YY_DECL
    #define YY_DECL int L3Lexer::lex(L3Parser::semantic_type *lval, location *lloc)

    #define YY_USER_INIT yylval = lval; yylloc = lloc;
%}

%option c++ noyywrap debug

%option yyclass="L3Lexer"
%option prefix="yy_l3_"

%%

%{
    using Token = L3Parser::token;
%}

[0-9]+                      { yylval->emplace<long>(std::stol(yytext)); return Token::num; }
\"([^\"](\\\")?)*\"         { yylval->emplace<std::string>(yytext); return Token::string; }
"+"                         { return Token::plus; }
"-"                         { return Token::minus; }
"*"                         { return Token::mul; }
"/"                         { return Token::div; }
"%"                         { return Token::mod; }
"="                         { return Token::equal; }
"+="                        { return Token::plus_equal; }
"-="                        { return Token::minus_equal; }
"*="                        { return Token::mul_equal; }
"/="                        { return Token::div_equal; }
"%="                        { return Token::mod_equal; }
"^="                        { return Token::pow_equal; }
".="                        { return Token::concat_equal; }
"("                         { return Token::lparen; }
")"                         { return Token::rparen; }
"["                         { return Token::lbracket; }
"]"                         { return Token::rbracket; }
"{"                         { return Token::lbrace; }
"}"                         { return Token::rbrace; }
","                         { return Token::comma; }
";"                         { return Token::semi; }
"or"                        { return Token::_or; }
"and"                       { return Token::_and; }
"not"                       { return Token::_not; }
"=="                        { return Token::equal_equal; }
"!="                        { return Token::not_equal; }
"<"                         { return Token::less; }
">"                         { return Token::greater; }
"<="                        { return Token::less_equal; }
">="                        { return Token::greater_equal; }
"if"                        { return Token::_if; }
"else"                      { return Token::_else; }
"while"                     { return Token::_while; }
"do"                        { return Token::_do; }
"return"                    { return Token::_return; }
"break"                     { return Token::_break; }
"continue"                  { return Token::_continue; }
"function"                  { return Token::function; }
"let"                       { return Token::let; }
"end"                       { return Token::end; }
"true"                      { return Token::_true; }
"false"                     { return Token::_false; }
"nil"                       { return Token::nil; }
[[:alpha:]̣_][[:alnum:]_]*   { yylval->emplace<std::string>(yytext); return Token::id; }

"#".* ;

[[:space:]] ;
. { std::fprintf(stderr, "Unknown character: %c\n", *yytext); }

%%
