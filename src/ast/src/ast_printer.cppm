module;

#include <format>
#include <iterator>

export module l3.ast.printer;

import l3.ast;

export namespace l3::ast {

template <
    typename T = char,
    std::output_iterator<T> OutputIterator = std::ostream_iterator<T>>
class AstPrinter : public AstVisitor<T, OutputIterator> {
  std::size_t depth = 0;

  void indent(OutputIterator &out) const {
    for (std::size_t i = 0; i < depth; ++i) {
      std::format_to(out, "▏ ");
    }
  }

  template <typename... Args>
  void format_indented(
      OutputIterator &out, std::format_string<Args...> fmt, Args &&...args
  ) {
    indent(out);
    std::format_to(out, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void format_indented_line(
      OutputIterator &out, std::format_string<Args...> fmt, Args &&...args
  ) {
    indent(out);
    std::format_to(out, fmt, std::forward<Args>(args)...);
    std::format_to(out, "\n");
  }

  struct DepthGuard {
    explicit DepthGuard(std::size_t &d) : depth(d) { ++depth; }
    DepthGuard(const DepthGuard &) = delete;
    DepthGuard(DepthGuard &&) = delete;
    DepthGuard &operator=(const DepthGuard &) = delete;
    DepthGuard &operator=(DepthGuard &&) = delete;
    ~DepthGuard() { --depth; }

  private:
    std::size_t &depth;
  };

public:
  void visit(const Array &node, OutputIterator &out) override {
    format_indented_line(out, "Array");
    DepthGuard guard(depth);
    for (const auto &element : node.get_elements()) {
      visit(element, out);
    }
  }

  void visit(const NameList &node, OutputIterator &out) override {
    for (const auto &name : node) {
      visit(name, out);
    }
  }

  void visit(const UnaryExpression &node, OutputIterator &out) override {
    format_indented_line(out, "UnaryExpression {}", node.get_op());
    DepthGuard guard(depth);
    visit(node.get_expression(), out);
  }

  void visit(const BinaryExpression &node, OutputIterator &out) override {
    format_indented_line(out, "BinaryExpression {}", node.get_op());
    DepthGuard guard(depth);
    visit(node.get_lhs(), out);
    visit(node.get_rhs(), out);
  }

  void visit(const LogicalExpression &node, OutputIterator &out) override {
    format_indented_line(out, "LogicalExpression {}", node.get_op());
    DepthGuard guard(depth);
    visit(node.get_lhs(), out);
    visit(node.get_rhs(), out);
  }

  void visit(const Comparison &node, OutputIterator &out) override {
    format_indented_line(out, "ChainedComparison");
    DepthGuard guard(depth);
    visit(node.get_start(), out);
    for (const auto &[op, rhs] : node.get_comparisons()) {
      format_indented_line(out, "{}", op);
      DepthGuard inner_guard(depth);
      visit(*rhs, out);
    }
  }

  void visit(const IndexExpression &node, OutputIterator &out) override {
    format_indented_line(out, "IndexExpression");
    DepthGuard guard(depth);
    visit(node.get_base(), out);
    visit(node.get_index(), out);
  }

  void visit(const FunctionCall &node, OutputIterator &out) override {
    format_indented_line(out, "FunctionCall");
    DepthGuard guard(depth);
    visit(node.get_name(), out);
    format_indented_line(out, "Arguments");
    {
      DepthGuard arg_guard(depth);
      for (const auto &arg : node.get_arguments()) {
        visit(arg, out);
      }
    }
  }

  void visit(const FunctionBody &node, OutputIterator &out) override {
    format_indented_line(out, "Parameters");
    {
      DepthGuard guard(depth);
      for (const Identifier &parameter : node.get_parameters()) {
        visit(parameter, out);
      }
    }
    visit(node.get_block(), out);
  }

  void visit(const IfBase &node, OutputIterator &out) override {
    format_indented_line(out, "Condition");
    {
      DepthGuard guard(depth);
      visit(node.get_condition(), out);
    }
    visit(node.get_block(), out);
  }

  void visit(const ElseIfList &node, OutputIterator &out) override {
    for (const auto &elseif : node.get_elseifs()) {
      visit(elseif, out);
    }
  }

  void visit(const IfStatement &node, OutputIterator &out) override {
    format_indented_line(out, "IfStatement");
    DepthGuard guard(depth);
    visit(node.get_base_if(), out);
    visit(node.get_elseif(), out);
    if (node.get_else_block()) {
      format_indented_line(out, "Else");
      DepthGuard else_guard(depth);
      visit(node.get_else_block().value(), out);
    }
  }

  void visit(const IfExpression &node, OutputIterator &out) override {
    format_indented_line(out, "IfExpression");
    DepthGuard guard(depth);
    visit(node.get_base_if(), out);
    visit(node.get_elseif(), out);
    format_indented_line(out, "Else");
    {
      DepthGuard else_guard(depth);
      visit(node.get_else_block(), out);
    }
  }

  void visit(const While &node, OutputIterator &out) override {
    format_indented_line(out, "While");
    DepthGuard guard(depth);
    format_indented_line(out, "Condition");
    {
      DepthGuard cond_guard(depth);
      visit(node.get_condition(), out);
    }
    format_indented_line(out, "Block");
    {
      DepthGuard block_guard(depth);
      visit(node.get_body(), out);
    }
  }

  void visit(const ForLoop &node, OutputIterator &out) override {
    format_indented_line(out, "ForLoop ({})", node.get_mutability());
    DepthGuard guard(depth);
    format_indented_line(out, "Variable");
    {
      DepthGuard var_guard(depth);
      visit(node.get_variable(), out);
    }
    format_indented_line(out, "Collection");
    {
      DepthGuard coll_guard(depth);
      visit(node.get_collection(), out);
    }
    format_indented_line(out, "Block");
    {
      DepthGuard block_guard(depth);
      visit(node.get_body(), out);
    }
  }

  void visit(const ReturnStatement &node, OutputIterator &out) override {
    format_indented_line(out, "Return");
    if (node.get_expression()) {
      DepthGuard guard(depth);
      visit(node.get_expression()->get(), out);
    }
  }

  void visit(const BreakStatement & /*break*/, OutputIterator &out) override {
    format_indented_line(out, "Break");
  }

  void
  visit(const ContinueStatement & /*continue*/, OutputIterator &out) override {
    format_indented_line(out, "Continue");
  }

  void visit(const LastStatement &node, OutputIterator &out) override {
    node.visit([this, &out](const auto &inner) { visit(inner, out); });
  }

  void visit(const Identifier &node, OutputIterator &out) override {
    format_indented_line(out, "Identifier '{}'", node.get_name());
  }

  void visit(const Nil & /*nil*/, OutputIterator &out) override {
    format_indented_line(out, "Nil");
  }

  void visit(const Boolean &node, OutputIterator &out) override {
    format_indented_line(out, "Boolean {}", node.get_value());
  }

  void visit(const Number &node, OutputIterator &out) override {
    format_indented_line(out, "Number {}", node.get_value());
  }

  void visit(const Float &node, OutputIterator &out) override {
    format_indented_line(out, "Float {}", node.get_value());
  }

  void visit(const String &node, OutputIterator &out) override {
    format_indented_line(out, "String \"{}\"", node.get_value());
  }

  void visit(const Literal &node, OutputIterator &out) override {
    node.visit([this, &out](const auto &inner) { visit(inner, out); });
  }

  void visit(const Variable &node, OutputIterator &out) override {
    node.visit([this, &out](const auto &inner) { visit(inner, out); });
  }

  void visit(const Expression &node, OutputIterator &out) override {
    node.visit([this, &out](const auto &inner) { visit(inner, out); });
  }

  void visit(const Statement &node, OutputIterator &out) override {
    node.visit([this, &out](const auto &inner) { visit(inner, out); });
  }

  void visit(const Block &node, OutputIterator &out) override {
    format_indented_line(out, "Block");
    DepthGuard guard(depth);
    for (const Statement &statement : node.get_statements()) {
      visit(statement, out);
    }
    if (node.get_last_statement()) {
      format_indented_line(out, "LastStatement");
      DepthGuard last_guard(depth);
      visit(*node.get_last_statement(), out);
    }
  }

  void visit(const NamedFunction &node, OutputIterator &out) override {
    format_indented_line(out, "NamedFunction");
    DepthGuard guard(depth);
    visit(node.get_name(), out);
    visit(node.get_body(), out);
  }

  void visit(const AnonymousFunction &node, OutputIterator &out) override {
    format_indented_line(out, "AnonymousFunction");
    DepthGuard guard(depth);
    visit(node.get_body(), out);
  }

  void visit(const Declaration &node, OutputIterator &out) override {
    format_indented_line(out, "Declaration {}", node.get_mutability());
    DepthGuard guard(depth);
    visit(node.get_names(), out);
    if (node.get_expression()) {
      visit(*node.get_expression(), out);
    }
  }

  void visit(const NameAssignment &node, OutputIterator &out) override {
    format_indented_line(out, "NameAssignment");
    DepthGuard guard(depth);
    visit(node.get_names(), out);
    visit(node.get_expression(), out);
  }

  void visit(const OperatorAssignment &node, OutputIterator &out) override {
    format_indented_line(out, "OperatorAssignment {}", node.get_operator());
    DepthGuard guard(depth);
    visit(node.get_variable(), out);
    visit(node.get_expression(), out);
  }
};

} // namespace l3::ast
