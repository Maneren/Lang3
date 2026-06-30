# Lang3 Project Structure

Lang3 is a C++26 project implementing a programming language interpreter/compiler using C++20 modules instead of headers. The project uses CMake for builds and organizes code into modular libraries.

## Directory Structure

- **src/**: Core source code with subdirectories for each library (e.g., `ast/`, `compiler/`, `vm/`). Each has its own `CMakeLists.txt`.
- **apps/lang3/**: Main `lang3` executable (single `src/main.cpp`).
- **test/unit/**: Unit tests (currently CLI parser tests).
- **test/snapshot/**: Snapshot tests (17 fixtures, each with expected ast/bytecode/output).
- **cmake/**: Build templates (`LibraryTemplate.cmake`, `ExecutableTemplate.cmake`, `TestTemplate.cmake`, `CommonUtils.cmake`).
- **build/**: Build artifacts (not committed).
- **examples/**: Sample `.l3` files (5 programs: blackjack, game of life, demo, eval, rle).
- **extras/vim/**: Vim syntax highlighting and filetype detection.
- **doc/**: LaTeX documentation with Graphviz diagrams.
- **external/**: Third-party dependencies (GTest via FetchContent).

## Key Modules and Relations

Modules are defined with `.cppm` files and use partitions for sub-components. Module names use the `l3.*` prefix (except `utils` and `cli`). Dependencies are mostly private, with public interfaces where needed.

- **utils** (module `utils`): Core utilities (visitors, accessors, ranges, types, match, debug, format). No dependencies. Uses partitions (`:debug`, `:format`, `:functional`, `:match`, `:ranges`, `:types`).
- **location** (module `l3.location`): Source location tracking. Depends on `utils` (private).
- **ast** (module `l3.ast`): Abstract Syntax Tree with partitions (e.g., `:assignment`, `:block`, `:expression`, `:literal`, `:function`, etc. — 28 module files). Depends on `utils` (private), `location` (public).
- **cli** (module `cli`): Command-line parsing. No dependencies.
- **parser**: Lexical/syntactic analysis (flex/bison generated). Depends on `ast`, `utils` (private).
- **runtime** (module `l3.runtime`): Runtime support (value model, GC, functions, chunks). Uses partitions (`:error`, `:formatting`, `:function`, `:gc_value`, `:chunk_list`, `:identifier`, `:mutability`, `:primitive`, `:ref_value`, `:storage`, `:value` — 12 module files). Depends on `ast`, `utils` (private).
- **bytecode** (module `l3.bytecode`): Bytecode definition and formatting. Depends on `runtime` (private).
- **compiler** (module `l3.compiler`): AST-to-bytecode compilation. Depends on `bytecode`, `runtime`, `ast`, `utils` (private).
- **vm** (module `l3.vm`): Virtual machine execution (includes `builtins` module). Depends on `bytecode`, `runtime`, `utils` (private).

**Dependency Chain**: `lang3` → `ast`, `cli`, `parser`, `compiler`, `bytecode`, `vm` (and their transitive deps: `runtime`, `location`, `utils`).

## Build and Commands

- Build system: CMake with Ninja Multi-Config, supports Debug/Release.
- Key `just` commands: `configure`, `build`, `run`, `test`, `snapshot-test`, `snapshot-update`, `clean`, `lint`, `format`.
- Requires CMake 3.31+, C++26 compiler, flex/bison.

## Snapshot Testing

- Snapshot fixtures live in `test/snapshot/inputs/*.l3`.
- Expected artifacts are stored in `test/snapshot/expected/<case>/{ast,bytecode,output}.txt`.
- Scope: each fixture snapshots parser/AST printing, compiler bytecode formatting, and VM stdout output.
- CMake targets: `snapshot_validate` for checks, `snapshot_update` for rewriting snapshots.
- Keep fixtures deterministic when possible; runtime failures are captured in `output.txt` as `RuntimeError: ...` lines.

## Module Organization

Modules use partitions for sub-components. `CMAKE_CXX_SCAN_FOR_MODULES` handles dependencies. No traditional headers for modules; uses `import std;` and custom imports. Most modules use the `l3.*` naming convention (`l3.ast`, `l3.runtime`, `l3.bytecode`, `l3.compiler`, `l3.vm`, `l3.location`), while `utils` and `cli` remain unqualified. Both `ast` and `runtime` extensively use partitions; `utils` also uses partitions for sub-components.

## Development Practices

- Use modern C++ features up to the version specified in `CMAKE_CXX_STANDARD`, for example:
  - `std::optional` and `std::variant`
  - `std::format` and `std::print`
  - `std::span` and `std::string_view`
  - `std::ranges` and `std::views`
- Prefer `auto` over explicit types except where necessary (like for number literals).
- Use `const` and `constexpr` where possible.
  - In particular, avoid (if possible) making a variable mutable just in order to declare and initialize it separately.
- Primary objective is clean and readable code, but keep the performance in mind as well.
- Try to match the formatting of the surrounding code.
- `ninja: no work to do.` means the target is up-to-date and has already been compiled without errors. Either touch a source file (preferably) or run `just configure` to force full recompilation.
