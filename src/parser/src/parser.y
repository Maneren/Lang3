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

%type <ast::UnaryExpression> UNARY
      <ast::BinaryExpression> BINARY
      <ast::Literal> LITERAL
      <ast::NameList> NAME_LIST
      <ast::ExpressionList> ARGUMENTS
      <ast::FunctionCall> FUNCTION_CALL
      <ast::FunctionBody> FUNCTION_BODY
      <ast::Expression> EXPRESSION
      <ast::ExpressionList> EXPRESSION_LIST
      <ast::Identifier> IDENTIFIER
      <ast::Variable> VAR
      <ast::IfClause> IF
      <ast::Statement> STATEMENT
      <ast::LastStatement> LAST_STATEMENT
      <ast::Block> BLOCK

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

UNARY: minus EXPRESSION
       { $$ = ast::UnaryExpression(ast::Unary::Minus, std::move($2)); }
     | _not  EXPRESSION
       { $$ = ast::UnaryExpression(ast::Unary::Not, std::move($2)); }
     | plus  EXPRESSION
       { $$ = ast::UnaryExpression(ast::Unary::Plus, std::move($2)); }

BINARY: EXPRESSION plus          EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::Plus, std::move($3)); }
      | EXPRESSION minus         EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::Minus, std::move($3)); }
      | EXPRESSION mul           EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::Multiply, std::move($3)); }
      | EXPRESSION div           EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::Divide, std::move($3)); }
      | EXPRESSION equal_equal   EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::Equal, std::move($3)); }
      | EXPRESSION not_equal     EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::NotEqual, std::move($3)); }
      | EXPRESSION less          EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::Less, std::move($3)); }
      | EXPRESSION less_equal    EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::LessEqual, std::move($3)); }
      | EXPRESSION greater       EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::Greater, std::move($3)); }
      | EXPRESSION greater_equal EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::GreaterEqual, std::move($3)); }
      | EXPRESSION _and          EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::And, std::move($3)); }
      | EXPRESSION _or           EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::Binary::Or, std::move($3)); }

LITERAL: nil    { $$ = ast::Literal(ast::Nil()); }
       | _true  { $$ = ast::Literal(ast::Boolean(true)); }
       | _false { $$ = ast::Literal(ast::Boolean(false)); }
       | num    { $$ = ast::Literal(ast::Number($1)); }
       | string { $$ = ast::Literal(ast::String(std::move($1))); }

IDENTIFIER: id { $$ = ast::Identifier(std::move($1)); }

VAR: IDENTIFIER { $$ = ast::Variable(std::move($1)); }
NAME_LIST: IDENTIFIER comma NAME_LIST { $3.push_back($1); $$ = $3; }
         | IDENTIFIER                 { $$ = ast::NameList{ $1 }; }
         | %empty             { $$ = ast::NameList{}; }

FUNCTION_BODY: lparen NAME_LIST rparen BLOCK end
               { $$ = ast::FunctionBody{ $2, std::make_shared<ast::Block>($4) }; }

FUNCTION_CALL: IDENTIFIER ARGUMENTS
               { $$ = ast::FunctionCall(std::move($1), std::move($2)); }

ARGUMENTS: lparen EXPRESSION_LIST rparen { $$ = $2; }

EXPRESSION: UNARY         { $$ = ast::Expression(std::move($1)); }
          | BINARY        { $$ = ast::Expression(std::move($1)); }
          // | TABLE                        { $$ = ast::Expression($1); }
          // | FUNCTION                     { $$ = ast::Expression($1); }
          | FUNCTION_CALL { $$ = ast::Expression(std::move($1)); }
          | VAR           { $$ = ast::Expression(std::move($1)); }
          | LITERAL       { $$ = ast::Expression(std::move($1)); }

EXPRESSION_LIST: EXPRESSION comma EXPRESSION_LIST { $3.push_back($1); $$ = ast::ExpressionList{ $3 }; }
               | EXPRESSION                       { $$ = ast::ExpressionList{ $1 }; }
               | %empty                           { $$ = ast::ExpressionList{}; }

IF: _if EXPRESSION then BLOCK end
    { $$ = ast::IfClause{ $2, std::make_shared<ast::Block>($4), std::nullopt }; }
  | _if EXPRESSION then BLOCK _else BLOCK end
    { $$ = ast::IfClause{ $2, std::make_shared<ast::Block>($4), std::make_shared<ast::Block>($6) }; }

STATEMENT:
           // VAR equal EXPRESSION
           // { $$ = ast::Statement(ast::Assignment(std::move($1), std::move($3))); }
           let IDENTIFIER equal EXPRESSION
           { $$ = ast::Statement(ast::Declaration(std::move($2), std::move($4))); }
         | FUNCTION_CALL
           { $$ = ast::Statement(std::move($1)); }

LAST_STATEMENT: _return EXPRESSION
                { $$ = ast::LastStatement(ast::ReturnStatement{ std::move($2) }); }
              | _continue           { $$ = ast::LastStatement(ast::ContinueStatement{}); }
              | _break              { $$ = ast::LastStatement(ast::BreakStatement{}); }

BLOCK: STATEMENT            { $$ = ast::Block{ std::move($1) }; }
     | STATEMENT BLOCK      { $2.add_statement(std::move($1)); $$ = std::move($2); }
     | STATEMENT semi BLOCK { $3.add_statement(std::move($1)); $$ = std::move($3); }
     | LAST_STATEMENT       { $$ = ast::Block{ std::move($1) }; }
     | LAST_STATEMENT semi  { $$ = ast::Block{ std::move($1) }; }

PROGRAM: BLOCK { program = $1; }

%%

namespace l3
{
    void L3Parser::error(const location_type &location, const std::string &message)
    {
        std::cerr << "Error at lines " << location << ": " << message << std::endl;
    }
}
