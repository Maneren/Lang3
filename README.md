# Lang3

A Lua-inspired interpreted language with a focus on quality of life features and
a better support for functional style of programming.

## Features

- Clean, whitespace independent syntax similar to Lua with hints of Python
- Optional semicolon separated statements
- Strong dynamic typing
- First-class functions and currying
- Garbage collector memory management
- Helpers for functional programming: \code{map}, \code{filter}, etc.

## Usage

### Prerequisites

To build the application, the following tools have to be installed
and available:

- **C++ 26** compliant compiler with C++ 20 module support[^1]
- [CMake](https://cmake.org/) 3.31 or newer
- [Ninja](https://ninja-build.org/)[^2]
- [Flex](https://flex.sourceforge.io/)
- [Bison](https://www.gnu.org/software/bison/)

[^1]: Compiler used for development was LLVM Clang, version 21.1.6. Newer versions of GCC and MSVC should probably work as well.

[^2]: Due to limitations of CMake's C++ module support, only the Ninja and Visual Studio backends are supported.

### Compilation

To build the release version using CMake, run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This will produce a binary called `lang3` in the `build/bin` directory.

### Running

The `lang3` binary has a simple command line interface that can be used to
run `.l3` files:

```bash
lang3 [options] [--] <file.l3>
```

If not file was specified or the `-` value was used, the application will
read from the standard input. The following options are available:

- `-t, --timings` – print timings of each stage
- `-d, --debug` – enable all debug logging
- `--debug-lexer` – enable lexer debug logging
- `--debug-parser` – enable parser debug logging
- `--debug-ast` – log parsed AST to console
- `--debug-ast-graph <file.dot>` – save parsed AST to a DOT file
- `--debug-vm` – enable VM debug logging

If any of the lexer, parser, or AST debug flags and none of the `debug` or
`debug-ast` flags are specified, the application will only parse the code
and exit without executing it.
