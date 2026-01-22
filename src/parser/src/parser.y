%require "3.2"
%language "c++"

%code requires {
    #include <ast/ast.hpp>

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

%verbose

%parse-param {L3Lexer &lexer}
%parse-param {const std::string &filename}
%parse-param {const bool debug}
%parse-param {ast::Program &program}

%initial-action
{
    #if YYDEBUG != 0
        set_debug_level(debug);
    #endif

    yyla.location.initialize(&filename, 1, 1);
};

%code {
    #include <string>
    #include <parser/lexer.hpp>

    #define yylex lexer.lex
}

%right lbracket
%right equal
%left  _or
%left  _and
%left  _not
%left  plus minus concat
%left  mul div mod
%right pow
%nonassoc equal_equal not_equal less less_equal greater greater_equal

%token _if _else _while _break _continue _return _for in _do _true _false then
       end nil function let equal _not lparen rparen lbrace rbrace lbracket
       rbracket comma semi dot concat colon plus_equal minus_equal mul_equal
       div_equal mod_equal pow_equal concat_equal elif mut
       <std::string> id
       <std::string> string
       <std::int64_t> num
       <double> fnum

%type <ast::UnaryExpression> UNARY
      <ast::BinaryExpression> BINARY
      <ast::Literal> LITERAL
      <ast::Array> ARRAY
      <ast::NameList> NAME_LIST
      <ast::NameList> PARAMETERS
      <ast::ExpressionList> ARGUMENTS
      <ast::FunctionCall> FUNCTION_CALL
      <ast::FunctionBody> FUNCTION_BODY
      <ast::AnonymousFunction> ANONYMOUS_FUNCTION
      <ast::Expression> PREFIX_EXPRESSION
      <ast::Expression> EXPRESSION
      <ast::ExpressionList> EXPRESSION_LIST
      <ast::Identifier> IDENTIFIER
      <ast::Variable> VAR
      <ast::IfBase> IF_BASE
      <ast::IfStatement> IF_STATEMENT
      <ast::IfExpression> IF_EXPRESSION
      <ast::ElseIfList> IF_ELSE
      <ast::AssignmentOperator> ASSIGNMENT_OPERATOR
      <ast::Assignment> ASSIGNMENT
      <ast::NameAssignment> NAME_ASSIGNMENT
      <ast::Declaration> DECLARATION
      <ast::NamedFunction> FUNCTION_DEFINITION
      <ast::Statement> STATEMENT
      <ast::Statement> SEMI_STATEMENT
      <ast::ReturnStatement> RETURN
      <ast::LastStatement> LAST_STATEMENT
      <ast::Block> BLOCK

%start PROGRAM

%%

UNARY: minus EXPRESSION
       { $$ = ast::UnaryExpression(ast::UnaryOperator::Minus, std::move($2)); }
     | _not  EXPRESSION
       { $$ = ast::UnaryExpression(ast::UnaryOperator::Not, std::move($2)); }
     | plus  EXPRESSION
       { $$ = ast::UnaryExpression(ast::UnaryOperator::Plus, std::move($2)); }

BINARY: EXPRESSION plus          EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::Plus, std::move($3)); }
      | EXPRESSION minus         EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::Minus, std::move($3)); }
      | EXPRESSION mul           EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::Multiply, std::move($3)); }
      | EXPRESSION div           EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::Divide, std::move($3)); }
      | EXPRESSION equal_equal   EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::Equal, std::move($3)); }
      | EXPRESSION not_equal     EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::NotEqual, std::move($3)); }
      | EXPRESSION less          EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::Less, std::move($3)); }
      | EXPRESSION less_equal    EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::LessEqual, std::move($3)); }
      | EXPRESSION greater       EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::Greater, std::move($3)); }
      | EXPRESSION greater_equal EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::GreaterEqual, std::move($3)); }
      | EXPRESSION _and          EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::And, std::move($3)); }
      | EXPRESSION _or           EXPRESSION
        { $$ = ast::BinaryExpression(std::move($1), ast::BinaryOperator::Or, std::move($3)); }

LITERAL: nil    { $$ = ast::Literal(ast::Nil()); }
       | _true  { $$ = ast::Literal(ast::Boolean(true)); }
       | _false { $$ = ast::Literal(ast::Boolean(false)); }
       | num    { $$ = ast::Literal(ast::Number($1)); }
       | fnum   { $$ = ast::Literal(ast::Float($1)); }
       | string { $$ = ast::Literal(ast::String($1)); }
       | ARRAY  { $$ = ast::Literal(std::move($1)); }

ARRAY: lbracket EXPRESSION_LIST rbracket { $$ = ast::Array{ std::move($2) }; }

IDENTIFIER: id { $$ = ast::Identifier(std::move($1)); }

VAR: IDENTIFIER { $$ = ast::Variable(std::move($1)); }
NAME_LIST: IDENTIFIER comma NAME_LIST { $$ = std::move($3.with_name(std::move($1))); }
         | IDENTIFIER                 { $$ = ast::NameList{ std::move($1) }; }
         | %empty                     { $$ = ast::NameList{}; }

PARAMETERS: lparen NAME_LIST rparen { $$ = std::move($2); }

FUNCTION_BODY: PARAMETERS BLOCK end
               { $$ = ast::FunctionBody{ std::move($1), std::move($2) }; }

ANONYMOUS_FUNCTION: function FUNCTION_BODY { $$ = ast::AnonymousFunction(std::move($2)); }

PREFIX_EXPRESSION: lparen EXPRESSION rparen                 { $$ = std::move($2); }
                 | EXPRESSION lbracket EXPRESSION rbracket
                   { $$ = ast::Expression(ast::IndexExpression(std::move($1), std::move($3))); }

FUNCTION_CALL: VAR ARGUMENTS
               { $$ = ast::FunctionCall(std::move($1), std::move($2)); }

ARGUMENTS: lparen EXPRESSION_LIST rparen { $$ = std::move($2); }

EXPRESSION: UNARY                    { $$ = ast::Expression(std::move($1)); }
          | BINARY                   { $$ = ast::Expression(std::move($1)); }
          | ANONYMOUS_FUNCTION       { $$ = ast::Expression(std::move($1)); }
          | FUNCTION_CALL            { $$ = ast::Expression(std::move($1)); }
          | VAR                      { $$ = ast::Expression(std::move($1)); }
          | PREFIX_EXPRESSION        { $$ = ast::Expression(std::move($1)); }
          | LITERAL                  { $$ = ast::Expression(std::move($1)); }
          | IF_EXPRESSION            { $$ = ast::Expression(std::move($1)); }

EXPRESSION_LIST: EXPRESSION comma EXPRESSION_LIST
                 { $$ = std::move($3.with_expression(std::move($1))); }
               | EXPRESSION
                 { $$ = ast::ExpressionList{ std::move($1) }; }
               | %empty
                 { $$ = ast::ExpressionList{}; }

IF_BASE: _if EXPRESSION then BLOCK
         { $$ = ast::IfBase{ std::move($2), std::move($4) }; }

IF_STATEMENT: IF_BASE IF_ELSE end
              { $$ = ast::IfStatement{ std::move($1), std::move($2) }; }
            | IF_BASE IF_ELSE _else BLOCK end
              { $$ = ast::IfStatement{ std::move($1), std::move($2), std::move($4) }; }

IF_EXPRESSION: IF_BASE IF_ELSE _else BLOCK end
               { $$ = ast::IfExpression{ std::move($1), std::move($2), std::move($4) }; }

IF_ELSE: elif EXPRESSION then BLOCK IF_ELSE
         { $5.emplace_back(ast::IfBase{ std::move($2), std::move($4) }); $$ = std::move($5); }
       | %empty { $$ = ast::ElseIfList{}; }

ASSIGNMENT_OPERATOR: plus_equal  { $$ = ast::AssignmentOperator::Plus; }
                   | minus_equal { $$ = ast::AssignmentOperator::Minus; }
                   | mul_equal   { $$ = ast::AssignmentOperator::Multiply; }
                   | div_equal   { $$ = ast::AssignmentOperator::Divide; }
                   | mod_equal   { $$ = ast::AssignmentOperator::Modulo; }
                   | pow_equal   { $$ = ast::AssignmentOperator::Power; }

NAME_ASSIGNMENT: NAME_LIST equal EXPRESSION
                 { $$ = ast::NameAssignment(std::move($1), std::move($3)); }

ASSIGNMENT: VAR ASSIGNMENT_OPERATOR EXPRESSION
            { $$ = ast::OperatorAssignment(std::move($1), $2, std::move($3)); }
          | NAME_LIST equal EXPRESSION
            { $$ = ast::NameAssignment(std::move($1), std::move($3)); }

DECLARATION: let NAME_ASSIGNMENT
             { $$ = ast::Declaration(std::move($2), true); }
           | let mut NAME_ASSIGNMENT
             { $$ = ast::Declaration(std::move($3), false); }

FUNCTION_DEFINITION: function IDENTIFIER FUNCTION_BODY
                     { $$ = ast::NamedFunction(std::move($2), std::move($3)); }

STATEMENT: ASSIGNMENT           { $$ = ast::Statement(std::move($1)); }
         | DECLARATION          { $$ = ast::Statement(std::move($1)); }
         | IF_STATEMENT         { $$ = ast::Statement(std::move($1)); }
         | FUNCTION_CALL        { $$ = ast::Statement(std::move($1)); }
         | FUNCTION_DEFINITION  { $$ = ast::Statement(std::move($1)); }

RETURN: _return EXPRESSION { $$ = ast::ReturnStatement{ std::move($2) }; }

LAST_STATEMENT: RETURN    { $$ = ast::LastStatement(std::move($1)); }
              | _continue { $$ = ast::LastStatement(ast::ContinueStatement{}); }
              | _break    { $$ = ast::LastStatement(ast::BreakStatement{}); }

SEMI_STATEMENT: STATEMENT      { $$ =  std::move($1); }
              | STATEMENT semi { $$ = std::move($1); }

BLOCK: SEMI_STATEMENT       { $$ = std::move($1); }
     | SEMI_STATEMENT BLOCK { $$ = std::move($2.with_statement(std::move($1))); }
     | LAST_STATEMENT       { $$ = ast::Block { std::move($1) }; }
     | LAST_STATEMENT semi  { $$ = ast::Block { std::move($1) }; }

PROGRAM: BLOCK { program = std::move($1); }

%%

namespace l3
{
    void L3Parser::error(const location_type &location, const std::string &message)
    {
        std::cerr << "Error at " << location << ": " << message << std::endl;
    }
}
