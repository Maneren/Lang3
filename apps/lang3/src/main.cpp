#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <parser/lexer.hpp>
#include <print>

import cli;
import l3.ast;
import l3.ast.printer;
import l3.ast.dot_printer;
import l3.vm;

namespace {

constexpr cli::Parser cli_parser() {
  return cli::Parser{}
      .flag("d", "debug")
      .flag("O", "optimize")
      .long_flag("debug-lexer")
      .long_flag("debug-parser")
      .long_flag("debug-ast")
      .long_option("debug-ast-graph")
      .long_flag("debug-vm")
      .long_flag("timings");
}

} // namespace

using namespace l3;

int main(int argc, char *argv[]) {
  const auto args = cli_parser().parse(argc, argv);

  if (!args) {
    std::println(std::cerr, "{}", args.error().what());
    return EXIT_FAILURE;
  }

  const auto positional = args->positional();

  const bool debug = args->has_flag("debug");
  const bool debug_lexer = debug || args->has_flag("debug-lexer");
  const bool debug_parser = debug || args->has_flag("debug-parser");
  const bool debug_ast = debug || args->has_flag("debug-ast");
  const bool debug_vm = debug || args->has_flag("debug-vm");
  const bool timings = debug || args->has_flag("timings");

  const auto debug_ast_graph = args->get_value("debug-ast-graph");

  std::istream *input = &std::cin;
  std::string filename = "<stdin>";

  std::ifstream input_file;

  if (!positional.empty() && positional[0] != "-") {
    input_file = std::ifstream{positional[0]};
    input = &input_file;
    filename = positional[0];
  }

  if (positional.size() > 1) {
    std::println(
        std::cerr, "Ignoring extra input files: {}", positional.subspan(1)
    );
  }

  auto start_time = std::chrono::steady_clock::now();

  if (debug_lexer && debug_parser) {
    std::println(std::cerr, "=== Lexer + Parser ===");
  } else if (debug_lexer) {
    std::println(std::cerr, "=== Lexer ===");
  } else if (debug_parser) {
    std::println(std::cerr, "=== Parser ===");
  }
  lexer::L3Lexer lexer(*input, debug_lexer);

  auto program = ast::Program{};

  parser::L3Parser parser(lexer, filename, debug_parser, program);
  const auto result = parser.parse();

  if (timings) {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    std::println(std::cerr, "Parsed to AST in {}ms", duration.count());
  }

  if (result != 0) {
    return result;
  }

  if (debug_ast) {
    std::println(std::cerr, "=== AST ===");
    ast::AstPrinter<char, std::ostreambuf_iterator<char>> printer;
    auto out_iter = std::ostreambuf_iterator<char>{std::cout};
    printer.visit(program, out_iter);
  }

  if (debug_ast_graph) {
    std::ofstream dot_file{std::string(*debug_ast_graph)};
    ast::DotPrinter<char, std::ostreambuf_iterator<char>> dot_printer;
    auto out_iter = std::ostreambuf_iterator<char>{dot_file};
    dot_printer.write_header(out_iter);
    dot_printer.visit(program, out_iter);
    dot_printer.write_footer(out_iter);
    std::println(std::cerr, "AST graph written to {}", *debug_ast_graph);
  }

  if ((debug_lexer || debug_parser || debug_ast || debug_ast_graph) &&
      !debug_vm) {
    return EXIT_SUCCESS;
  }

  if (debug_vm) {
    std::println(std::cerr, "=== VM ===");
  }

  vm::VM vm{debug_vm};

  start_time = std::chrono::steady_clock::now();

  vm.execute(program);

  if (timings) {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    std::println(std::cerr, "Executed in {}ms", duration.count());
  }

  return EXIT_SUCCESS;
}
