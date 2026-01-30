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

%type
      <AnonymousFunction> ANONYMOUS_FUNCTION
      <Array> ARRAY
      <Assignment> ASSIGNMENT
      <AssignmentOperator> ASSIGNMENT_OPERATOR
      <BinaryExpression> BINARY
      <Block> BLOCK
      <Declaration> DECLARATION
      <ElseIfList> IF_ELSE
      <Expression> EXPRESSION
      <Expression> INDEX
      <ExpressionList> ARGUMENTS
      <ExpressionList> EXPRESSION_LIST
      <Expression> PREFIX_EXPRESSION
      <Expression> PRIMARY_EXPRESSION
      <ForLoop> FOR
      <FunctionBody> FUNCTION_BODY
      <FunctionCall> FUNCTION_CALL
      <Identifier> IDENTIFIER
      <IfBase> IF_BASE
      <IfExpression> IF_EXPRESSION
      <IfStatement> IF_STATEMENT
      <std::optional<Expression>> INITIALIZER
      <LastStatement> LAST_STATEMENT
      <Literal> LITERAL
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
       | fnum    { $$ = { Float { $1 } }; }
       | string  { $$ = { String { $1 } }; }
       | ARRAY   { $$ = { std::move($1) }; }

ARRAY: lbracket EXPRESSION_LIST rbracket { $$ = { std::move($2) }; }

// Identifiers and Variables
IDENTIFIER: id { $$ = { std::move($1) }; }

INDEX: lbracket PRIMARY_EXPRESSION rbracket { $$ = { std::move($2) }; }

VAR: IDENTIFIER { $$ = { std::move($1) }; }
   | VAR INDEX  { $$ = { IndexExpression { std::move($1), std::move($2) } }; }

NAME_LIST: IDENTIFIER comma NAME_LIST
           { $$ = std::move($3.with_name(std::move($1))); }
         | IDENTIFIER
           { $$ = { std::move($1) }; }

MULTIPLE_NAME_LIST: IDENTIFIER comma NAME_LIST
                    { $$ = std::move($3.with_name(std::move($1))); }

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

PREFIX_EXPRESSION: FUNCTION_CALL                 { $$ = { std::move($1) }; }
                 | VAR                           { $$ = { std::move($1) }; }
                 | lparen PRIMARY_EXPRESSION rparen { $$ = std::move($2); }

EXPRESSION: UNARY                    { $$ = { std::move($1) }; }
          | BINARY                   { $$ = { std::move($1) }; }
          | ANONYMOUS_FUNCTION       { $$ = { std::move($1) }; }
          | PREFIX_EXPRESSION        { $$ = { std::move($1) }; }
          | LITERAL                  { $$ = { std::move($1) }; }

PRIMARY_EXPRESSION: EXPRESSION      { $$ = std::move($1); }
                  | IF_EXPRESSION   { $$ = { std::move($1) }; }

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
         | IF_STATEMENT         { $$ = { std::move($1) }; }
         | FUNCTION_CALL        { $$ = { std::move($1) }; }
         | FUNCTION_DEFINITION  { $$ = { std::move($1) }; }
         | WHILE                { $$ = { std::move($1) }; }

SEMI_STATEMENT: STATEMENT      { $$ = std::move($1); }
              | STATEMENT semi { $$ = std::move($1); }

BLOCK: SEMI_STATEMENT       { $$ = std::move($1); }
     | SEMI_STATEMENT BLOCK { $$ = std::move($2.with_statement(std::move($1))); }
     | LAST_STATEMENT       { $$ = { std::move($1) }; }
     | LAST_STATEMENT semi  { $$ = { std::move($1) }; }

// Program Root
PROGRAM: BLOCK { program = std::move($1); }
