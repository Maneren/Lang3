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
%define parse.lac full

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

%right equal
%left  _or
%left  _and
%left  _not
%left  equal_equal not_equal less less_equal greater greater_equal
%left  plus minus concat
%left  mul div mod
%right pow

%token _if _else _while _break _continue _return _for in _do _true _false then
       end nil function let equal _not lparen rparen lbrace rbrace lbracket
       rbracket comma semi plus_equal minus_equal mul_equal div_equal mod_equal
       pow_equal elif mut step dot_dot dot_dot_equal
       <std::string> id
       <std::string> string
       <std::int64_t> num

%type
      <AnonymousFunction> ANONYMOUS_FUNCTION
      <Array> ARRAY
      <Assignment> ASSIGNMENT
      <AssignmentOperator> ASSIGNMENT_OPERATOR
      <BinaryExpression> BINARY
      <Block> BLOCK
      <Comparison> COMPARISON
      <ComparisonOperator> COMPARISON_OP
      <Declaration> DECLARATION
      <ElseIfList> IF_ELSE
      <Expression> ATOMIC_EXPRESSION
      <Expression> EXPRESSION
      <Expression> INDEX
      <std::optional<Expression>> INITIALIZER
      <ExpressionList> ARGUMENTS
      <ExpressionList> EXPRESSION_LIST
      <Expression> PREFIX_EXPRESSION
      <Expression> PRIMARY_EXPRESSION
      <ForLoop> FOR
      <RangeForLoop> RANGE_FOR
      <std::optional<Expression>> STEP_CLAUSE
      <RangeOperator> RANGE_OPERATOR
      <FunctionBody> FUNCTION_BODY
      <FunctionCall> FUNCTION_CALL
      <Identifier> IDENTIFIER
      <IfBase> IF_BASE
      <IfExpression> IF_EXPRESSION
      <IfStatement> IF_STATEMENT
      <LastStatement> LAST_STATEMENT
      <Literal> LITERAL
      <LogicalExpression> LOGICAL
      <Mutability> MUTABILITY
      <NamedFunction> FUNCTION_DEFINITION
      <NameList> MULTIPLE_NAME_LIST
      <NameList> NAME_LIST
      <NameList> PARAMETERS
      <ReturnStatement> RETURN
      <Statement> SEMI_STATEMENT
      <Statement> STATEMENT
      <UnaryExpression> UNARY
      <Variable> VAR
      <While> WHILE

%start PROGRAM

%%

// Literals
LITERAL: nil     { $$ = { Nil {} }; }
       | _true   { $$ = { Boolean { true } }; }
       | _false  { $$ = { Boolean { false } }; }
       | num     { $$ = { Number { $1 } }; }
       | string  { $$ = { String { $1 } }; }
       | ARRAY   { $$ = { std::move($1) }; }

ARRAY: lbracket EXPRESSION_LIST rbracket { $$ = { std::move($2) }; }

// Identifiers and Variables
IDENTIFIER: id { $$ = { std::move($1) }; }

INDEX: lbracket PRIMARY_EXPRESSION rbracket { $$ = { std::move($2) }; }

VAR: IDENTIFIER { $$ = { std::move($1) }; }
   | VAR INDEX  { $$ = { IndexExpression { std::move($1), std::move($2) } }; }

NAME_LIST: NAME_LIST comma IDENTIFIER
           { $$ = std::move($1.with_name(std::move($3))); }
         | IDENTIFIER
           { $$ = { std::move($1) }; }

MULTIPLE_NAME_LIST: NAME_LIST comma IDENTIFIER
                    { $$ = std::move($1.with_name(std::move($3))); }

// Expressions
UNARY: minus ATOMIC_EXPRESSION
       { $$ = { UnaryOperator::Minus, std::move($2) }; }
     | _not ATOMIC_EXPRESSION
       { $$ = { UnaryOperator::Not, std::move($2) }; }
     | plus ATOMIC_EXPRESSION
       { $$ = { UnaryOperator::Plus, std::move($2) }; }

BINARY: ATOMIC_EXPRESSION plus ATOMIC_EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Plus, std::move($3) }; }
      | ATOMIC_EXPRESSION minus ATOMIC_EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Minus, std::move($3) }; }
      | ATOMIC_EXPRESSION mul ATOMIC_EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Multiply, std::move($3) }; }
      | ATOMIC_EXPRESSION div ATOMIC_EXPRESSION
        { $$ = { std::move($1), BinaryOperator::Divide, std::move($3) }; }

COMPARISON_OP: equal_equal    { $$ = ComparisonOperator::Equal; }
             | not_equal      { $$ = ComparisonOperator::NotEqual; }
             | less           { $$ = ComparisonOperator::Less; }
             | less_equal     { $$ = ComparisonOperator::LessEqual; }
             | greater        { $$ = ComparisonOperator::Greater; }
             | greater_equal  { $$ = ComparisonOperator::GreaterEqual; }

ATOMIC_EXPRESSION: UNARY              { $$ = { std::move($1) }; }
                 | BINARY             { $$ = { std::move($1) }; }
                 | LITERAL            { $$ = { std::move($1) }; }
                 | PREFIX_EXPRESSION  { $$ = { std::move($1) }; }

COMPARISON: COMPARISON COMPARISON_OP ATOMIC_EXPRESSION
            { if (!$1.add_comparison($2, std::move($3))) {
                throw syntax_error(
                  @$,
                  "syntax error, mixed equality and inequality operators in chained comparison"
                );
              }
              $$ = std::move($1); }
          | ATOMIC_EXPRESSION COMPARISON_OP ATOMIC_EXPRESSION
            { $$ = { std::move($1), $2, std::move($3) }; }

LOGICAL: EXPRESSION _and EXPRESSION
         { $$ = { std::move($1), LogicalOperator::And, std::move($3) }; }
       | EXPRESSION _or EXPRESSION
         { $$ = { std::move($1), LogicalOperator::Or, std::move($3) }; }

PREFIX_EXPRESSION: FUNCTION_CALL                 { $$ = { std::move($1) }; }
                 | VAR                           { $$ = { std::move($1) }; }
                 | lparen PRIMARY_EXPRESSION rparen { $$ = std::move($2); }

EXPRESSION: ATOMIC_EXPRESSION { $$ = { std::move($1) }; }
          | COMPARISON        { $$ = { std::move($1) }; }
          | LOGICAL           { $$ = { std::move($1) }; }
          | error             { $$ = {}; yynerrs_++; }

PRIMARY_EXPRESSION: EXPRESSION      { $$ = std::move($1); }
                  | IF_EXPRESSION   { $$ = { std::move($1) }; }
                  | ANONYMOUS_FUNCTION  { $$ = { std::move($1) }; }

EXPRESSION_LIST: PRIMARY_EXPRESSION comma EXPRESSION_LIST
                 { $$ = std::move($3.with_expression(std::move($1))); }
               | PRIMARY_EXPRESSION
                 { $$ = { std::move($1) }; }
               | %empty
                 { $$ = {}; }

// Functions
PARAMETERS: lparen NAME_LIST rparen { $$ = std::move($2); }
          | lparen rparen           { $$ = {}; }

FUNCTION_BODY: PARAMETERS BLOCK end { $$ = { std::move($1), std::move($2) }; }

ANONYMOUS_FUNCTION: function FUNCTION_BODY { $$ = { std::move($2) }; }

FUNCTION_CALL: IDENTIFIER ARGUMENTS { $$ = { std::move($1), std::move($2) }; }

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

// Loops
WHILE: _while EXPRESSION _do BLOCK end
       { $$ = { std::move($2), std::move($4) }; }

FOR: _for MUTABILITY IDENTIFIER in EXPRESSION _do BLOCK end
     { $$ = { std::move($3), std::move($5), std::move($7), std::move($2) }; }

RANGE_OPERATOR: dot_dot        { $$ = RangeOperator::Exclusive; }
              | dot_dot_equal  { $$ = RangeOperator::Inclusive; }

STEP_CLAUSE: step ATOMIC_EXPRESSION  { $$ = std::move($2); }
           | %empty                  { $$ = std::nullopt; }

RANGE_FOR: _for MUTABILITY IDENTIFIER in ATOMIC_EXPRESSION RANGE_OPERATOR ATOMIC_EXPRESSION STEP_CLAUSE _do BLOCK end
           { $$ = {
              std::move($2), // mutability
              std::move($3), // identifier
              std::move($5), // start
              $6,            // range operator
              std::move($7), // end
              std::move($8), // step
              std::move($10) // block
             }; }

// Assignments and Declarations
ASSIGNMENT_OPERATOR: plus_equal  { $$ = AssignmentOperator::Plus; }
                   | minus_equal { $$ = AssignmentOperator::Minus; }
                   | mul_equal   { $$ = AssignmentOperator::Multiply; }
                   | div_equal   { $$ = AssignmentOperator::Divide; }
                   | mod_equal   { $$ = AssignmentOperator::Modulo; }
                   | pow_equal   { $$ = AssignmentOperator::Power; }
                   | equal       { $$ = AssignmentOperator::Assign; }

ASSIGNMENT: VAR ASSIGNMENT_OPERATOR PRIMARY_EXPRESSION
            { $$ = OperatorAssignment { std::move($1), $2, std::move($3) }; }
          | MULTIPLE_NAME_LIST equal PRIMARY_EXPRESSION
            { $$ = NameAssignment { std::move($1), std::move($3) }; }

INITIALIZER: equal PRIMARY_EXPRESSION { $$ = std::move($2); }
           | %empty                   { $$ = std::nullopt; }

MUTABILITY: mut     { $$ = Mutability::Mutable; }
          | %empty  { $$ = Mutability::Immutable; }

DECLARATION: let MUTABILITY NAME_LIST INITIALIZER
             { $$ = { std::move($3), std::move($4), std::move($2) }; }

// Control Statements
RETURN: _return PRIMARY_EXPRESSION { $$ = { std::move($2) }; }
      | _return                    {}

LAST_STATEMENT: RETURN    { $$ = { std::move($1) }; }
              | _continue { $$ = { ContinueStatement {} }; }
              | _break    { $$ = { BreakStatement {} }; }

// Statements and Blocks
STATEMENT: ASSIGNMENT           { $$ = { std::move($1) }; }
         | DECLARATION          { $$ = { std::move($1) }; }
         | FOR                  { $$ = { std::move($1) }; }
         | RANGE_FOR            { $$ = { std::move($1) }; }
         | IF_STATEMENT         { $$ = { std::move($1) }; }
         | FUNCTION_CALL        { $$ = { std::move($1) }; }
         | FUNCTION_DEFINITION  { $$ = { std::move($1) }; }
         | WHILE                { $$ = { std::move($1) }; }
         | error                { $$ = {}; yynerrs_++; }

SEMI_STATEMENT: STATEMENT      { $$ = std::move($1); }
              | STATEMENT semi { $$ = std::move($1); }

BLOCK: SEMI_STATEMENT       { $$ = std::move($1); }
     | SEMI_STATEMENT BLOCK { $$ = std::move($2.with_statement(std::move($1))); }
     | LAST_STATEMENT       { $$ = { std::move($1) }; }
     | LAST_STATEMENT semi  { $$ = { std::move($1) }; }

// Program Root
PROGRAM: BLOCK { if (yynerrs_ > 0) YYABORT;
                 program = std::move($1); }
