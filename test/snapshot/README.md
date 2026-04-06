# Snapshot Test Suite

This directory contains parser/compiler/vm snapshot tests for `.l3` programs.

## Layout

- `inputs/*.l3`: source files used as snapshot fixtures.
- `expected/<case>/ast.txt`: pretty-printed AST snapshot.
- `expected/<case>/bytecode.txt`: compiled bytecode snapshot.
- `expected/<case>/output.txt`: runtime stdout snapshot.

Each input file produces one case directory in `expected/`.

## Validate snapshots

With CMake target:

```bash
cmake --build build --config Debug --target snapshot_validate
```

With CTest filter:

```bash
ctest --test-dir build -C Debug -R ParserCompilerVmSnapshots --output-on-failure
```

## Update snapshots

With CMake target:

```bash
cmake --build build --config Debug --target snapshot_update
```

Or directly with env var:

```bash
L3_UPDATE_SNAPSHOTS=1 ctest --test-dir build -C Debug -R ParserCompilerVmSnapshots
```

## Notes

- Keep fixtures deterministic. Avoid `random()`, `sleep()`, or `input()` in snapshot inputs.
- The AST formatter uses Unicode indentation glyphs; snapshot files preserve that output.
