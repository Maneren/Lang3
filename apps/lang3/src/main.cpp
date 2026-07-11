#include <lexer/lexer.hpp>

import std;
import l3.cli;
import l3.ast;
import l3.ast.printer;
import l3.ast.dot_printer;
import l3.bytecode;
import l3.compiler;
import l3.runtime;
import l3.vm;

namespace {

constexpr cli::Parser cli_parser() {
  return cli::Parser{}
      .program_name("lang3")
      .program_description("A Lang3 programming language interpreter.")
      .flag("d", "debug", "Enable all debug options")
      .flag("O", "optimize", "Enable optimizations")
      .long_flag("debug-lexer", "Debug the lexer")
      .long_flag("debug-parser", "Debug the parser")
      .long_flag("debug-ast", "Debug the AST")
      .long_option("debug-ast-graph", "Output AST graph to a DOT file")
      .long_flag("debug-vm", "Debug the VM")
      .long_flag("debug-bytecode", "Debug the bytecode")
      .long_flag("timings", "Show execution timings");
}

struct Debug {
  bool lexer = false;
  bool parser = false;
  bool ast = false;
  std::optional<std::string_view> ast_graph = std::nullopt;
  bool vm = false;
  bool bytecode = false;
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
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time
    );
    std::println(std::cerr, "Parsed to AST in {}μs", duration.count());
  }

  if (result != 0) {
    return std::nullopt;
  }

  return program;
}

std::optional<bytecode::ProgramBytecode>
compile_ast(const ast::Program &program, const Debug &debug) {
  const auto start_time = std::chrono::steady_clock::now();
  bytecode::ProgramBytecode program_bytecode;
  try {
    compiler::Compiler compiler(program_bytecode);
    compiler.compile(program);
  } catch (const compiler::CompileError &error) {
    std::println(std::cerr, "Internal compiler error: {}", error.what());
    return std::nullopt;
  }

  if (debug.timings) {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time
    );
    std::println(std::cerr, "Compiled to bytecode in {}μs", duration.count());
  }

  return program_bytecode;
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
      .bytecode = debug_flag || args->has_flag("debug-bytecode"),
      .timings = debug_flag || args->has_flag("timings"),
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

  auto compiled = compile_ast(program, debug);
  if (!compiled) {
    return EXIT_FAILURE;
  }
  auto &program_bytecode = *compiled;

  if (debug.bytecode) {
    std::print(std::cerr, "{}", program_bytecode);

    if (!debug.vm) {
      return EXIT_SUCCESS;
    }
  }

  vm::BytecodeVM vm{debug.vm};
  const auto start_time = std::chrono::steady_clock::now();
  try {
    vm.execute(program_bytecode);
  } catch (runtime::RuntimeError &error) {
    std::println(std::cerr, "{}", error.format_error());
    return EXIT_FAILURE;
  }

  if (debug.timings) {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    std::println(std::cerr, "Executed in {}ms", duration.count());
  }

  return EXIT_SUCCESS;
}
