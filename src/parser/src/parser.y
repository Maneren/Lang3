%require "3.2"
%language "c++"

%code requires {
    #include "parser/ast.hpp"

    namespace l3 {
      class L3Lexer;
    }
}

%define api.namespace {l3}
%define api.parser.class {L3Parser}
%define api.value.type variant

%locations
%define api.location.file none

%define parse.error detailed
%define parse.trace

%header
%verbose

%parse-param {L3Lexer &lexer}
%parse-param {const bool debug}
%parse-param {ast::Program &program}

%initial-action
{
    #if YYDEBUG != 0
        set_debug_level(debug);
    #endif
};

%code {
    #include <string>
    #include <print>

    #include "lexer/lexer.hpp"

    #define yylex lexer.lex
}

%token _if _else _while _break _continue _return _for in _do _true _false then
       end nil function let plus minus mul div equal equal_equal not_equal less
       less_equal greater greater_equal _not _and _or lparen rparen lbrace
       rbrace lbracket rbracket comma semi dot mod concat colon
       <std::string> id
       <std::string> string
       <size_t> num

%type // <ast::Unary> UNARY
      <ast::BinaryExpression> BINARY
      <ast::Literal> LITERAL
      <ast::NameList> NAME_LIST
      <ast::ExpressionList> ARGUMENTS
      <ast::FunctionCall> FUNCTION_CALL
      <ast::FunctionBody> FUNCTION_BODY
      <ast::Expression> EXPRESSION
      <ast::ExpressionList> EXPRESSION_LIST
      <ast::IfClause> IF
      <ast::Statement> STATEMENT
      <ast::LastStatement> LAST_STATEMENT
      <ast::Block> BLOCK
      PROGRAM

%right equal
%left  _or
%left  _and
%left  _not
%left  equal_equal not_equal less less_equal greater greater_equal
%left  plus minus concat
%left  mul div
%right pow

%start PROGRAM

%%

// UNARY: minus  { $$ = ast::Unary::Minus; }
//      | _not   { $$ = ast::Unary::Not; }
//      | plus   { $$ = ast::Unary::Plus; }

BINARY: EXPRESSION plus          EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::Plus, $3); }
      | EXPRESSION minus         EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::Minus, $3); }
      | EXPRESSION mul           EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::Multiply, $3); }
      | EXPRESSION div           EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::Divide, $3); }
      | EXPRESSION equal_equal   EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::Equal, $3); }
      | EXPRESSION not_equal     EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::NotEqual, $3); }
      | EXPRESSION less          EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::Less, $3); }
      | EXPRESSION less_equal    EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::LessEqual, $3); }
      | EXPRESSION greater       EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::Greater, $3); }
      | EXPRESSION greater_equal EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::GreaterEqual, $3); }
      | EXPRESSION _and          EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::And, $3);  }
      | EXPRESSION _or           EXPRESSION
        { $$ = ast::BinaryExpression($1, ast::Binary::Or, $3);  }

LITERAL: nil    { $$ = ast::Literal(ast::Nil()); }
       | _true  { $$ = ast::Literal(ast::True()); }
       | _false { $$ = ast::Literal(ast::False()); }
       | num    { $$ = ast::Literal(ast::Num($1)); }
       | string { $$ = ast::Literal(ast::String($1)); }

NAME_LIST: id comma NAME_LIST { $3.push_back($1); $$ = $3; }
         | id                 { $$ = ast::NameList{ $1 }; }
         | %empty             { $$ = ast::NameList{}; }

FUNCTION_BODY: lparen NAME_LIST rparen BLOCK end
               { $$ = ast::FunctionBody{ $2, std::make_shared<ast::Block>($4) }; }

FUNCTION_CALL: id ARGUMENTS { $$ = ast::FunctionCall($1, $2); }

ARGUMENTS: lparen EXPRESSION_LIST rparen { $$ = $2; }

EXPRESSION:
          // UNARY EXPRESSION
          //   { $$ = ast::Expression(ast::UnaryExpression($1, std::make_shared<ast::Expression>($2))); }
           BINARY { $$ = ast::Expression(std::move($1)); }
          | LITERAL                      { $$ = ast::Expression(ast::Literal($1)); }

EXPRESSION_LIST: EXPRESSION comma EXPRESSION_LIST { $3.push_back($1); $$ = ast::ExpressionList{ $3 }; }
               | EXPRESSION                       { $$ = ast::ExpressionList{ $1 }; }
               | %empty                           { $$ = ast::ExpressionList{}; }

IF: _if EXPRESSION then BLOCK end
    { $$ = ast::IfClause{ $2, std::make_shared<ast::Block>($4), std::nullopt }; }
  | _if EXPRESSION then BLOCK _else BLOCK end
    { $$ = ast::IfClause{ $2, std::make_shared<ast::Block>($4), std::make_shared<ast::Block>($6) }; }

STATEMENT: id equal EXPRESSION       { $$ = ast::Statement(ast::Assignment($1, $3)); }
         | FUNCTION_CALL              { $$ = ast::Statement(ast::Expression($1)); }
         | IF                         { $$ = ast::Statement($1); }
         | function id FUNCTION_BODY  { $$ = ast::Statement(ast::NamedFunction($2, $3)); }
         | let id equal EXPRESSION    { $$ = ast::Statement(ast::Declaration($2, $4)); }

LAST_STATEMENT: _return EXPRESSION  { $$ = ast::ReturnStatement{ $2 }; }
              | _continue           { $$ = ast::ContinueStatement{}; }
              | _break              { $$ = ast::BreakStatement{}; }

BLOCK: STATEMENT            { $$ = ast::Block{ { $1 }, std::nullopt }; }
     | STATEMENT semi BLOCK { $3.statements.push_back($1); $$ = $3; }
     | LAST_STATEMENT       { $$ = ast::Block{ {}, std::make_optional($1) }; }
     | LAST_STATEMENT semi  { $$ = ast::Block{ {}, std::make_optional($1) }; }
     | %empty               { $$ = ast::Block{ {}, std::nullopt }; std::cerr << "empty block" << std::endl; }

PROGRAM: BLOCK { program = $1; }

%%

namespace l3
{
    void L3Parser::error(const location_type &location, const std::string &message)
    {
        std::cerr << "Error at lines " << location << ": " << message << std::endl;
    }
}
