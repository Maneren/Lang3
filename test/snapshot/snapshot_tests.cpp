#include <gtest/gtest.h>

#include <lexer/lexer.hpp>
#include <unistd.h>

import std;

import l3.ast;
import l3.ast.ast_printer;
import l3.bytecode;
import l3.compiler;
import l3.vm;

namespace {

using namespace l3;
namespace fs = std::filesystem;

const fs::path kSnapshotRoot = fs::path{L3_SNAPSHOT_ROOT};
const fs::path kInputsDir = kSnapshotRoot / "inputs";
const fs::path kExpectedDir = kSnapshotRoot / "expected";

struct SnapshotCase {
  std::string name;
  std::string filename;
  fs::path input;
};

std::string sanitize_name(const std::string &name) {
  std::string out;
  out.reserve(name.size());
  for (const char ch : name) {
    const auto c = static_cast<unsigned char>(ch);
    if (std::isalnum(c) != 0) {
      out.push_back(ch);
    } else {
      out.push_back('_');
    }
  }
  return out;
}

std::vector<SnapshotCase> discover_cases() {
  std::vector<SnapshotCase> cases;

  if (!fs::exists(kInputsDir)) {
    return cases;
  }

  for (const auto &entry : fs::recursive_directory_iterator{kInputsDir}) {
    if (!entry.is_regular_file()) {
      continue;
    }

    if (entry.path().extension() != ".l3") {
      continue;
    }

    const auto relative = fs::relative(entry.path(), kInputsDir);
    auto name = sanitize_name(relative.generic_string());
    if (name.size() >= 3 && name.ends_with("_l3")) {
      name.resize(name.size() - 3);
    }

    cases.push_back(
        SnapshotCase{
            .name = std::move(name),
            .filename = relative.generic_string(),
            .input = entry.path()
        }
    );
  }

  std::ranges::sort(
      cases, [](const SnapshotCase &lhs, const SnapshotCase &rhs) {
        return lhs.name < rhs.name;
      }
  );

  return cases;
}

std::optional<ast::Program>
parse_ast(const std::string &source, const std::string &filename) {
  std::istringstream input{source};
  lexer::L3Lexer lexer(input, false);

  auto program = ast::Program{};
  parser::L3Parser parser(lexer, filename, false, program);
  if (parser.parse() != 0) {
    return std::nullopt;
  }

  return program;
}

std::string render_ast(const ast::Program &program) {
  std::string ast_text;
  ast::AstPrinter<char, std::back_insert_iterator<std::string>> printer;
  auto out = std::back_inserter(ast_text);
  printer.visit(program, out);
  return ast_text;
}

std::string render_bytecode(const bytecode::ProgramBytecode &program) {
  return std::format("{}", program);
}

std::string run_and_capture_stdout(bytecode::ProgramBytecode &program) {
  struct StdoutRestore {
    int saved_fd = -1;
    ~StdoutRestore() {
      if (saved_fd >= 0) {
        std::fflush(stdout);
        std::cout.flush();
        ::dup2(saved_fd, STDOUT_FILENO);
        ::close(saved_fd);
      }
    }
  } restore;

  std::FILE *tmp = std::tmpfile();
  EXPECT_NE(
      tmp, nullptr
  ) << "Failed to create temporary file for stdout capture";
  if (tmp == nullptr) {
    return {};
  }

  std::fflush(stdout);
  std::cout.flush();
  restore.saved_fd = ::dup(STDOUT_FILENO);
  EXPECT_GE(restore.saved_fd, 0) << "Failed to duplicate stdout fd";
  if (restore.saved_fd < 0) {
    std::fclose(tmp);
    return {};
  }

  const int tmp_fd = ::fileno(tmp);
  EXPECT_GE(tmp_fd, 0) << "Failed to get temporary file descriptor";
  if (tmp_fd < 0) {
    std::fclose(tmp);
    return {};
  }

  const int redirect_rc = ::dup2(tmp_fd, STDOUT_FILENO);
  EXPECT_GE(redirect_rc, 0) << "Failed to redirect stdout";
  if (redirect_rc < 0) {
    std::fclose(tmp);
    return {};
  }

  std::optional<std::string> runtime_error;
  try {
    vm::BytecodeVM runtime_vm{false};
    runtime_vm.execute(program);
  } catch (const std::exception &ex) {
    runtime_error = std::format("RuntimeError: {}\n", ex.what());
  } catch (...) {
    std::fclose(tmp);
    throw;
  }

  std::fflush(stdout);
  std::cout.flush();

  std::string output;
  std::rewind(tmp);
  std::array<char, 4096> buffer{};
  while (true) {
    const std::size_t read = std::fread(buffer.data(), 1, buffer.size(), tmp);
    if (read == 0) {
      break;
    }
    output.append(buffer.data(), read);
  }

  if (runtime_error.has_value()) {
    output.append(*runtime_error);
  }

  std::fclose(tmp);
  return output;
}

bool should_update() {
  if (const char *value = std::getenv("L3_UPDATE_SNAPSHOTS")) {
    return std::string_view{value} == "1";
  }
  return false;
}

void ensure_parent_exists(const fs::path &path) {
  if (const auto parent = path.parent_path(); !parent.empty()) {
    fs::create_directories(parent);
  }
}

void write_text_file(const fs::path &path, const std::string &content) {
  ensure_parent_exists(path);
  std::ofstream file{path, std::ios::binary};
  ASSERT_TRUE(file.is_open()) << "Cannot write file: " << path;
  file << content;
}

std::string read_text_file(const fs::path &path) {
  std::ifstream file{path, std::ios::binary};
  EXPECT_TRUE(file.is_open()) << "Missing snapshot file: " << path;
  return std::string{
      std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}
  };
}

std::string normalize_newlines(std::string text) {
  std::string normalized;
  normalized.reserve(text.size());

  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\r') {
      if (i + 1 < text.size() && text[i + 1] == '\n') {
        continue;
      }
      normalized.push_back('\n');
      continue;
    }

    normalized.push_back(text[i]);
  }

  return normalized;
}

void assert_or_update_snapshot(
    const std::string &test_name,
    const fs::path &path,
    const std::string &actual,
    bool update
) {
  if (update) {
    write_text_file(path, actual);
    return;
  }

  const std::string expected = read_text_file(path);
  EXPECT_STREQ(
      normalize_newlines(expected).data(), normalize_newlines(actual).data()
  ) << "Snapshot mismatch in "
    << test_name << " for " << path;
}

struct SnapshotArtifacts {
  std::string ast;
  std::string bytecode;
  std::string output;
};

SnapshotArtifacts build_artifacts(const SnapshotCase &test_case) {
  const std::string source = read_text_file(test_case.input);

  auto program_opt = parse_ast(source, test_case.filename);
  EXPECT_TRUE(program_opt.has_value())
      << "Parser failed for " << test_case.input;
  if (!program_opt) {
    return {};
  }

  SnapshotArtifacts artifacts;
  artifacts.ast = render_ast(*program_opt);

  bytecode::ProgramBytecode bytecode_program;
  compiler::Compiler compiler{bytecode_program};
  compiler.compile(*program_opt);

  artifacts.bytecode = render_bytecode(bytecode_program);
  artifacts.output = run_and_capture_stdout(bytecode_program);

  return artifacts;
}

class SnapshotTests : public ::testing::TestWithParam<SnapshotCase> {};

std::string
snapshot_case_name(const testing::TestParamInfo<SnapshotCase> &info) {
  return info.param.name;
}

TEST_P(SnapshotTests, MatchesSnapshots) {
  const auto &test_case = GetParam();
  const bool update = should_update();

  const auto artifacts = build_artifacts(test_case);

  const fs::path case_dir = kExpectedDir / test_case.name;
  assert_or_update_snapshot(
      test_case.name, case_dir / "ast.txt", artifacts.ast, update
  );
  assert_or_update_snapshot(
      test_case.name, case_dir / "bytecode.txt", artifacts.bytecode, update
  );
  assert_or_update_snapshot(
      test_case.name, case_dir / "output.txt", artifacts.output, update
  );
}

INSTANTIATE_TEST_SUITE_P(
    ParserCompilerVmSnapshots,
    SnapshotTests,
    ::testing::ValuesIn(discover_cases()),
    snapshot_case_name
);

} // namespace
