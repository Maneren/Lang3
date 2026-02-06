export module l3.ast:visitor;

export namespace l3::ast {

class Array;
class NameList;
class UnaryExpression;
class BinaryExpression;
class LogicalExpression;
class Comparison;
class IndexExpression;
class FunctionCall;
class FunctionBody;
class IfBase;
class ElseIfList;
class IfStatement;
class IfExpression;
class While;
class ForLoop;
class RangeForLoop;
class ReturnStatement;
class BreakStatement;
class ContinueStatement;
class LastStatement;
class Identifier;
class Nil;
class Boolean;
class Number;
class Float;
class String;
class Literal;
class Variable;
class Expression;
class Statement;
class Block;
class NamedFunction;
class AnonymousFunction;
class Declaration;
class NameAssignment;
class OperatorAssignment;

template <
    typename T = char,
    std::output_iterator<T> OutputIterator = std::ostream_iterator<T>>
class AstVisitor {
public:
  virtual void visit(const Array &node, OutputIterator &out) = 0;
  virtual void visit(const NameList &node, OutputIterator &out) = 0;
  virtual void visit(const UnaryExpression &node, OutputIterator &out) = 0;
  virtual void visit(const BinaryExpression &node, OutputIterator &out) = 0;
  virtual void visit(const LogicalExpression &node, OutputIterator &out) = 0;
  virtual void visit(const Comparison &node, OutputIterator &out) = 0;
  virtual void visit(const IndexExpression &node, OutputIterator &out) = 0;
  virtual void visit(const FunctionCall &node, OutputIterator &out) = 0;
  virtual void visit(const FunctionBody &node, OutputIterator &out) = 0;
  virtual void visit(const IfBase &node, OutputIterator &out) = 0;
  virtual void visit(const ElseIfList &node, OutputIterator &out) = 0;
  virtual void visit(const IfStatement &node, OutputIterator &out) = 0;
  virtual void visit(const IfExpression &node, OutputIterator &out) = 0;
  virtual void visit(const While &node, OutputIterator &out) = 0;
  virtual void visit(const ForLoop &node, OutputIterator &out) = 0;
  virtual void visit(const RangeForLoop &node, OutputIterator &out) = 0;
  virtual void visit(const ReturnStatement &node, OutputIterator &out) = 0;
  virtual void visit(const BreakStatement &node, OutputIterator &out) = 0;
  virtual void visit(const ContinueStatement &node, OutputIterator &out) = 0;
  virtual void visit(const LastStatement &node, OutputIterator &out) = 0;
  virtual void visit(const Identifier &node, OutputIterator &out) = 0;
  virtual void visit(const Nil &node, OutputIterator &out) = 0;
  virtual void visit(const Boolean &node, OutputIterator &out) = 0;
  virtual void visit(const Number &node, OutputIterator &out) = 0;
  virtual void visit(const Float &node, OutputIterator &out) = 0;
  virtual void visit(const String &node, OutputIterator &out) = 0;
  virtual void visit(const Literal &node, OutputIterator &out) = 0;
  virtual void visit(const Variable &node, OutputIterator &out) = 0;
  virtual void visit(const Expression &node, OutputIterator &out) = 0;
  virtual void visit(const Statement &node, OutputIterator &out) = 0;
  virtual void visit(const Block &node, OutputIterator &out) = 0;
  virtual void visit(const NamedFunction &node, OutputIterator &out) = 0;
  virtual void visit(const AnonymousFunction &node, OutputIterator &out) = 0;
  virtual void visit(const Declaration &node, OutputIterator &out) = 0;
  virtual void visit(const NameAssignment &node, OutputIterator &out) = 0;
  virtual void visit(const OperatorAssignment &node, OutputIterator &out) = 0;
};

} // namespace l3::ast
