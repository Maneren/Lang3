#include <chrono>
#include <fstream>
#include <lexer/lexer.hpp>
#include <thread>

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

struct Debug {
  bool lexer = false;
  bool parser = false;
  bool ast = false;
  std::optional<std::string_view> ast_graph = std::nullopt;
  bool vm = false;
  bool timings = false;
};

using namespace l3;

std::optional<ast::Program> parse_ast(
    std::istream *input, const std::string &filename, const Debug &debug
) {
  auto start_time = std::chrono::steady_clock::now();

  if (debug.lexer && debug.parser) {
    std::println(std::cerr, "=== Lexer + Parser ===");
  } else if (debug.lexer) {
    std::println(std::cerr, "=== Lexer ===");
  } else if (debug.parser) {
    std::println(std::cerr, "=== Parser ===");
  }
  lexer::L3Lexer lexer(*input, debug.lexer);

  auto program = ast::Program{};

  parser::L3Parser parser(lexer, filename, debug.parser, program);
  const auto result = parser.parse();

  if (debug.timings) {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    std::println(std::cerr, "Parsed to AST in {}ms", duration.count());
  }

  if (result != 0) {
    return std::nullopt;
  }

  return program;
}

} // namespace

int main(int argc, char *argv[]) {
  const auto args = cli_parser().parse(argc, argv);

  if (!args) {
    std::println(std::cerr, "{}", args.error().what());
    return EXIT_FAILURE;
  }

  const auto positional = args->positional();

  const bool debug_flag = args->has_flag("debug");

  Debug debug{
      .lexer = debug_flag || args->has_flag("debug-lexer"),
      .parser = debug_flag || args->has_flag("debug-parser"),
      .ast = debug_flag || args->has_flag("debug-ast"),
      .ast_graph = args->get_value("debug-ast-graph"),
      .vm = debug_flag || args->has_flag("debug-vm"),
      .timings = debug_flag || args->has_flag("timings")
  };

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

  auto program_opt = parse_ast(input, filename, debug);
  if (!program_opt) {
    return EXIT_FAILURE;
  }
  const auto &program = program_opt.value();

  if (debug.ast) {
    std::println(std::cerr, "=== AST ===");
    ast::AstPrinter<char, std::ostreambuf_iterator<char>> printer;
    auto out_iter = std::ostreambuf_iterator<char>{std::cout};
    printer.visit(program, out_iter);
  }

  if (debug.ast_graph) {
    std::ofstream dot_file{std::string(*debug.ast_graph)};
    ast::DotPrinter<char, std::ostreambuf_iterator<char>> dot_printer;
    auto out_iter = std::ostreambuf_iterator<char>{dot_file};
    dot_printer.write_graph(program, out_iter);
    std::println(std::cerr, "AST graph written to {}", *debug.ast_graph);
  }

  if ((debug.lexer || debug.parser || debug.ast || debug.ast_graph) &&
      !debug.vm) {
    return EXIT_SUCCESS;
  }

  if (debug.vm) {
    std::println(std::cerr, "=== VM ===");
  }

  vm::VM vm{debug.vm};

  const auto start_time = std::chrono::steady_clock::now();

  vm.execute(program);

  if (debug.timings) {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    std::println(std::cerr, "Executed in {}ms", duration.count());
  }

  return EXIT_SUCCESS;
}
