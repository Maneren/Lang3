%require "3.2"
%language "c++"

%code requires {
    import l3.ast;

    namespace l3::lexer {
      class L3Lexer;
    }

    using namespace l3::ast;
}

%define api.namespace {l3::parser}
%define api.parser.class {L3Parser}
%define api.value.type variant

%locations
%define api.location.file none

%define parse.error detailed
%define parse.trace

%verbose

%parse-param {lexer::L3Lexer &lexer}
%parse-param {const std::string &filename}
%parse-param {const bool debug}
%parse-param {Program &program}

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

%type <UnaryExpression> UNARY
      <BinaryExpression> BINARY
      <Literal> LITERAL
      <Array> ARRAY
      <NameList> NAME_LIST
      <NameList> PARAMETERS
      <ExpressionList> ARGUMENTS
      <FunctionCall> FUNCTION_CALL
      <FunctionBody> FUNCTION_BODY
      <AnonymousFunction> ANONYMOUS_FUNCTION
      <Expression> PREFIX_EXPRESSION
      <Expression> EXPRESSION
      <ExpressionList> EXPRESSION_LIST
      <Identifier> IDENTIFIER
      <Variable> VAR
      <IfBase> IF_BASE
      <IfStatement> IF_STATEMENT
      <IfExpression> IF_EXPRESSION
      <ElseIfList> IF_ELSE
      <AssignmentOperator> ASSIGNMENT_OPERATOR
      <Assignment> ASSIGNMENT
      <NameAssignment> NAME_ASSIGNMENT
      <Declaration> DECLARATION
      <NamedFunction> FUNCTION_DEFINITION
      <Statement> STATEMENT
      <Statement> SEMI_STATEMENT
      <ReturnStatement> RETURN
      <LastStatement> LAST_STATEMENT
      <Block> BLOCK

%start PROGRAM

%%

// Literals
LITERAL: nil     { $$ = { Nil {} }; }
       | _true   { $$ = { Boolean { true } }; }
       | _false  { $$ = { Boolean { false } }; }
       | num     { $$ = { Number { $1 } }; }
       | fnum    { $$ = { Float { $1 } }; }
       | string  { $$ = { String { $1 } }; }
       | ARRAY   { $$ = { std::move($1) }; }

ARRAY: lbracket EXPRESSION_LIST rbracket { $$ = { std::move($2) }; }

// Identifiers and Variables
IDENTIFIER: id { $$ = { std::move($1) }; }

VAR: IDENTIFIER { $$ = { std::move($1) }; }

NAME_LIST: IDENTIFIER comma NAME_LIST
           { $$ = std::move($3.with_name(std::move($1))); }
         | IDENTIFIER
           { $$ = { std::move($1) }; }
         | %empty
           { $$ = {}; }

// Expressions
UNARY: minus EXPRESSION
       { $$ = UnaryExpression { UnaryOperator::Minus, std::move($2) }; }
     | _not EXPRESSION
       { $$ = UnaryExpression { UnaryOperator::Not, std::move($2) }; }
     | plus EXPRESSION
       { $$ = UnaryExpression { UnaryOperator::Plus, std::move($2) }; }

BINARY: EXPRESSION plus EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Plus, std::move($3) }; }
      | EXPRESSION minus EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Minus, std::move($3) }; }
      | EXPRESSION mul EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Multiply, std::move($3) }; }
      | EXPRESSION div EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Divide, std::move($3) }; }
      | EXPRESSION equal_equal EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Equal, std::move($3) }; }
      | EXPRESSION not_equal EXPRESSION
        { $$ = { std::move($1), BinaryOperator::NotEqual, std::move($3) }; }
      | EXPRESSION less EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Less, std::move($3) }; }
      | EXPRESSION less_equal EXPRESSION
        { $$ = { std::move($1), BinaryOperator::LessEqual, std::move($3) }; }
      | EXPRESSION greater EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Greater, std::move($3) }; }
      | EXPRESSION greater_equal EXPRESSION
        { $$ = { std::move($1), BinaryOperator::GreaterEqual, std::move($3) }; }
      | EXPRESSION _and EXPRESSION
        { $$ = { std::move($1), BinaryOperator::And, std::move($3) }; }
      | EXPRESSION _or EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Or, std::move($3) }; }

PREFIX_EXPRESSION: lparen EXPRESSION rparen
                   { $$ = std::move($2); }
                 | EXPRESSION lbracket EXPRESSION rbracket
                   { $$ = { IndexExpression { std::move($1), std::move($3) } }; }

EXPRESSION: UNARY                    { $$ = { std::move($1) }; }
          | BINARY                   { $$ = { std::move($1) }; }
          | ANONYMOUS_FUNCTION       { $$ = { std::move($1) }; }
          | FUNCTION_CALL            { $$ = { std::move($1) }; }
          | VAR                      { $$ = { std::move($1) }; }
          | PREFIX_EXPRESSION        { $$ = { std::move($1) }; }
          | LITERAL                  { $$ = { std::move($1) }; }
          | IF_EXPRESSION            { $$ = { std::move($1) }; }

EXPRESSION_LIST: EXPRESSION comma EXPRESSION_LIST
                 { $$ = std::move($3.with_expression(std::move($1))); }
               | EXPRESSION
                 { $$ = { std::move($1) }; }
               | %empty
                 { $$ = {}; }

// Functions
PARAMETERS: lparen NAME_LIST rparen { $$ = std::move($2); }

FUNCTION_BODY: PARAMETERS BLOCK end { $$ = { std::move($1), std::move($2) }; }

ANONYMOUS_FUNCTION: function FUNCTION_BODY { $$ = { std::move($2) }; }

FUNCTION_CALL: VAR ARGUMENTS { $$ = { std::move($1), std::move($2) }; }

FUNCTION_DEFINITION: function IDENTIFIER FUNCTION_BODY
                     { $$ = { std::move($2), std::move($3) }; }

ARGUMENTS: lparen EXPRESSION_LIST rparen { $$ = std::move($2); }

// Control Flow
IF_BASE: _if EXPRESSION then BLOCK { $$ = { std::move($2), std::move($4) }; }

IF_ELSE: elif EXPRESSION then BLOCK IF_ELSE
         { $$ = std::move($5.with_if({ std::move($2), std::move($4) })); }
       | %empty {}

IF_STATEMENT: IF_BASE IF_ELSE end
              { $$ = { std::move($1), std::move($2) }; }
            | IF_BASE IF_ELSE _else BLOCK end
              { $$ = { std::move($1), std::move($2), std::move($4) }; }

IF_EXPRESSION: IF_BASE IF_ELSE _else BLOCK end
               { $$ = { std::move($1), std::move($2), std::move($4) }; }

// Assignments and Declarations
ASSIGNMENT_OPERATOR: plus_equal  { $$ = AssignmentOperator::Plus; }
                   | minus_equal { $$ = AssignmentOperator::Minus; }
                   | mul_equal   { $$ = AssignmentOperator::Multiply; }
                   | div_equal   { $$ = AssignmentOperator::Divide; }
                   | mod_equal   { $$ = AssignmentOperator::Modulo; }
                   | pow_equal   { $$ = AssignmentOperator::Power; }

NAME_ASSIGNMENT: NAME_LIST equal EXPRESSION
                 { $$ = { std::move($1), std::move($3) }; }

ASSIGNMENT: VAR ASSIGNMENT_OPERATOR EXPRESSION
            { $$ = OperatorAssignment { std::move($1), $2, std::move($3) }; }
          | NAME_LIST equal EXPRESSION
            { $$ = NameAssignment { std::move($1), std::move($3) }; }

DECLARATION: let NAME_ASSIGNMENT      { $$ = { std::move($2), Mutability::Immutable }; }
           | let mut NAME_ASSIGNMENT  { $$ = { std::move($3), Mutability::Mutable }; }

// Control Statements
RETURN: _return EXPRESSION { $$ = { std::move($2) }; }
      | _return            {}

LAST_STATEMENT: RETURN    { $$ = { std::move($1) }; }
              | _continue { $$ = { ContinueStatement {} }; }
              | _break    { $$ = { BreakStatement {} }; }

// Statements and Blocks
STATEMENT: ASSIGNMENT           { $$ = { std::move($1) }; }
         | DECLARATION          { $$ = { std::move($1) }; }
         | IF_STATEMENT         { $$ = { std::move($1) }; }
         | FUNCTION_CALL        { $$ = { std::move($1) }; }
         | FUNCTION_DEFINITION  { $$ = { std::move($1) }; }

SEMI_STATEMENT: STATEMENT      { $$ = std::move($1); }
              | STATEMENT semi { $$ = std::move($1); }

BLOCK: SEMI_STATEMENT       { $$ = std::move($1); }
     | SEMI_STATEMENT BLOCK { $$ = std::move($2.with_statement(std::move($1))); }
     | LAST_STATEMENT       { $$ = { std::move($1) }; }
     | LAST_STATEMENT semi  { $$ = { std::move($1) }; }

// Program Root
PROGRAM: BLOCK { program = std::move($1); }
