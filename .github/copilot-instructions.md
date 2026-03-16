# AGENTS.md file

## Dev environment tips

- Use `just` for compilation, running, and testing
  - Specifically: `just build`, `just run`, and `just test`
- Before compilation, configurations may be needed: `just configure`
  - If there seems to be a stale error, use `just clean configure`
- After finishing a feature, run `just format`
- Do not touch git in any way. Do not commit, push, or search the history unless
  asked. Only allowed subcommands are status and diff.

## Testing instructions

- Test currently only test the CLI library
- To test the parser and VM, run it with the demo input:
  - `just run Debug examples/demo.l3`
  - if needed, create custom inputs in the `examples` directory

## Project structure

- `apps`: contains application code
- `src`: contains a bunch of libraries used in the application
  - each library has a `src` directory with the source code and a `include`
    directory with the header files
  - most of them are C++ 20 modules with the `.cppm` extension
  - `parser` uses flex and bison to generate a lexer and parser
- `test`: contains tests

## Coding style

- Prefer modern C++ features up to C++ 26
- Prefer ranges to iterators and range adapters over for loops
- Pay attention to const-correctness and utilizing constexpr when possible
- When adding new features, make sure to identify existing helpers and use the
  to prevent duplication. If there are no existing helpers, create a new
  helper if necessary and refactor the existing code.
