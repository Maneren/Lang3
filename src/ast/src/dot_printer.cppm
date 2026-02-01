module;

#include <format>
#include <iterator>

export module l3.ast.dot_printer;

import l3.ast;

export namespace l3::ast {

template <typename T = char, typename OutputIterator = std::ostream_iterator<T>>
class DotPrinter : public AstVisitor<T, OutputIterator> {
  std::size_t node_id = 0;

  std::size_t get_next_id() { return node_id++; }

  template <typename... Args>
  void write_node(
      OutputIterator &out,
      std::size_t id,
      std::format_string<Args...> fmt,
      Args &&...args
  ) {
    std::format_to(
        out,
        "  n{} [label=\"{}\"];\n",
        id,
        std::format(fmt, std::forward<Args>(args)...)
    );
  }

  void write_edge(OutputIterator &out, std::size_t from, std::size_t to) {
    std::format_to(out, "  n{} -> n{};\n", from, to);
  }

  void write_edge_labeled(
      OutputIterator &out,
      std::size_t from,
      std::size_t to,
      std::string_view label
  ) {
    std::format_to(out, "  n{} -> n{} [label=\"{}\"];\n", from, to, label);
  }

public:
  void visit(const Array &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "Array");
    for (const auto &element : node.get_elements()) {
      write_edge(out, id, node_id);
      visit(element, out);
    }
  }

  void visit(const NameList &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "NameList");
    for (const auto &name : node) {
      write_edge(out, id, node_id);
      visit(name, out);
    }
  }

  void visit(const UnaryExpression &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "UnaryExpression\\n{}", node.get_op());
    write_edge(out, id, node_id);
    visit(node.get_expression(), out);
  }

  void visit(const BinaryExpression &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "BinaryExpression\\n{}", node.get_op());
    write_edge_labeled(out, id, node_id, "lhs");
    visit(node.get_lhs(), out);
    write_edge_labeled(out, id, node_id, "rhs");
    visit(node.get_rhs(), out);
  }

  void visit(const LogicalExpression &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "LogicalExpression\\n{}", node.get_op());
    write_edge_labeled(out, id, node_id, "lhs");
    visit(node.get_lhs(), out);
    write_edge_labeled(out, id, node_id, "rhs");
    visit(node.get_rhs(), out);
  }

  void visit(const Comparison &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "ChainedComparison");
    size_t start_id = node_id;
    visit(node.get_start(), out);
    write_edge_labeled(out, id, start_id, "start");
    for (const auto &[op, rhs] : node.get_comparisons()) {
      auto op_id = get_next_id();
      write_edge(out, id, op_id);
      write_node(out, op_id, "{}", op);
      write_edge(out, id, node_id);
      visit(*rhs, out);
    }
  }

  void visit(const IndexExpression &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "IndexExpression");
    write_edge_labeled(out, id, node_id, "base");
    visit(node.get_base(), out);
    write_edge_labeled(out, id, node_id, "index");
    visit(node.get_index(), out);
  }

  void visit(const FunctionCall &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "FunctionCall");
    write_edge_labeled(out, id, node_id, "name");
    visit(node.get_name(), out);

    auto args_id = get_next_id();
    write_edge(out, id, args_id);
    write_node(out, args_id, "Arguments");

    for (const auto &arg : node.get_arguments()) {
      write_edge(out, id, node_id);
      visit(arg, out);
    }
  }

  void visit(const FunctionBody &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "FunctionBody");

    auto params_id = get_next_id();
    write_edge(out, id, params_id);
    write_node(out, params_id, "Parameters");

    for (const Identifier &parameter : node.get_parameters()) {
      write_edge(out, id, node_id);
      visit(parameter, out);
    }

    write_edge_labeled(out, id, node_id, "body");
    visit(node.get_block(), out);
  }

  void visit(const IfBase &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "IfBase");

    write_edge_labeled(out, id, node_id, "condition");
    visit(node.get_condition(), out);

    write_edge_labeled(out, id, node_id, "block");
    visit(node.get_block(), out);
  }

  void visit(const ElseIfList &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "ElseIfList");
    for (const auto &elseif : node.get_elseifs()) {
      write_edge(out, id, node_id);
      visit(elseif, out);
    }
  }

  void visit(const IfStatement &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "IfStatement");

    write_edge_labeled(out, id, node_id, "if");
    visit(node.get_base_if(), out);

    write_edge_labeled(out, id, node_id, "elseif");
    visit(node.get_elseif(), out);

    if (node.get_else_block()) {
      write_edge_labeled(out, id, node_id, "else");
      visit(node.get_else_block().value(), out);
    }
  }

  void visit(const IfExpression &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "IfExpression");

    write_edge_labeled(out, id, node_id, "if");
    visit(node.get_base_if(), out);

    write_edge_labeled(out, id, node_id, "elseif");
    visit(node.get_elseif(), out);

    write_edge_labeled(out, id, node_id, "else");
    visit(node.get_else_block(), out);
  }

  void visit(const While &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "While");

    write_edge_labeled(out, id, node_id, "condition");
    visit(node.get_condition(), out);

    write_edge_labeled(out, id, node_id, "body");
    visit(node.get_body(), out);
  }

  void visit(const ForLoop &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "ForLoop\\n{}", node.get_mutability());

    write_edge_labeled(out, id, node_id, "variable");
    visit(node.get_variable(), out);

    write_edge_labeled(out, id, node_id, "collection");
    visit(node.get_collection(), out);

    write_edge_labeled(out, id, node_id, "body");
    visit(node.get_body(), out);
  }

  void visit(const RangeForLoop &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(
        out,
        id,
        "RangeForLoop\\n{}\\n{}",
        node.get_mutability(),
        node.get_range_type()
    );

    write_edge_labeled(out, id, node_id, "variable");
    visit(node.get_variable(), out);

    write_edge_labeled(out, id, node_id, "start");
    visit(node.get_start(), out);

    write_edge_labeled(out, id, node_id, "end");
    visit(node.get_end(), out);

    if (const auto &step = node.get_step()) {
      write_edge_labeled(out, id, node_id, "step");
      visit(**step, out);
    }

    write_edge_labeled(out, id, node_id, "body");
    visit(node.get_body(), out);
  }

  void visit(const ReturnStatement &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "Return");
    if (node.get_expression()) {
      write_edge(out, id, node_id);
      visit(node.get_expression()->get(), out);
    }
  }

  void visit(const BreakStatement & /*break*/, OutputIterator &out) override {
    write_node(out, get_next_id(), "Break");
  }

  void
  visit(const ContinueStatement & /*continue*/, OutputIterator &out) override {
    write_node(out, get_next_id(), "Continue");
  }

  void visit(const LastStatement &node, OutputIterator &out) override {
    node.visit([this, &out](const auto &inner) { visit(inner, out); });
  }

  void visit(const Identifier &node, OutputIterator &out) override {
    write_node(out, get_next_id(), "Identifier\\n'{}'", node.get_name());
  }

  void visit(const Nil & /*nil*/, OutputIterator &out) override {
    write_node(out, get_next_id(), "Nil");
  }

  void visit(const Boolean &node, OutputIterator &out) override {
    write_node(out, get_next_id(), "Boolean\\n{}", node.get_value());
  }

  void visit(const Number &node, OutputIterator &out) override {
    write_node(out, get_next_id(), "Number\\n{}", node.get_value());
  }

  void visit(const Float &node, OutputIterator &out) override {
    write_node(out, get_next_id(), "Float\\n{}", node.get_value());
  }

  void visit(const String &node, OutputIterator &out) override {
    write_node(out, get_next_id(), R"(String\n\"{}\")", node.get_value());
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
    size_t id = get_next_id();
    write_node(out, id, "Block");
    for (const Statement &statement : node.get_statements()) {
      write_edge(out, id, node_id);
      visit(statement, out);
    }
    if (node.get_last_statement()) {
      write_edge_labeled(out, id, node_id, "last");
      visit(*node.get_last_statement(), out);
    }
  }

  void visit(const NamedFunction &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "NamedFunction");
    write_edge_labeled(out, id, node_id, "name");
    visit(node.get_name(), out);
    write_edge_labeled(out, id, node_id, "body");
    visit(node.get_body(), out);
  }

  void visit(const AnonymousFunction &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "AnonymousFunction");
    write_edge(out, id, node_id);
    visit(node.get_body(), out);
  }

  void visit(const Declaration &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "Declaration\\n{}", node.get_mutability());

    write_edge_labeled(out, id, node_id, "names");
    visit(node.get_names(), out);

    if (node.get_expression().has_value()) {
      write_edge_labeled(out, id, node_id, "init");
      visit(node.get_expression().value(), out);
    }
  }

  void visit(const NameAssignment &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "NameAssignment");

    write_edge_labeled(out, id, node_id, "names");
    visit(node.get_names(), out);

    write_edge_labeled(out, id, node_id, "value");
    visit(node.get_expression(), out);
  }

  void visit(const OperatorAssignment &node, OutputIterator &out) override {
    size_t id = get_next_id();
    write_node(out, id, "OperatorAssignment\\n{}", node.get_operator());

    write_edge_labeled(out, id, node_id, "variable");
    visit(node.get_variable(), out);

    write_edge_labeled(out, id, node_id, "value");
    visit(node.get_expression(), out);
  }

  void write_header(OutputIterator &out) {
    std::format_to(
        out,
        "digraph AST {{\n"
        R"(node [shape=box, style="rounded", ordering=out];)"
        "  rankdir=TB;"
        "  nodesep=0.5;"
        "  ranksep=1.0;"
        "  splines=true;"
        R"(fontname="Helvetica,Roboto,sans-serif";)"
        R"(node [fontname="Helvetica,Roboto,sans-serif"];)"
        R"(edge [fontname="Helvetica,Roboto,sans-serif"];)"
        "\n"
    );
    std::format_to(out, "\n");
  }

  void write_footer(OutputIterator &out) { std::format_to(out, "}}\n"); }
};

} // namespace l3::ast
