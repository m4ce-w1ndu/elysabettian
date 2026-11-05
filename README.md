# Elysabettian (Plain C Implementation)

A high-performance runtime, bytecode virtual machine, and native JIT compiler architecture for the **Elysabettian** scripting language, ported to **Standard C (C11)**.

---

## Overview

Elysabettian is a lightweight, dynamically typed scripting language featuring clean, JavaScript-like syntax. This repository represents the pure C implementation of the runtime—porting the architecture from [`elysabettian-cpp`](../elysabettian-cpp) to idiomatic, portable C11 with minimal overhead and clear data-driven structures.

### Key Goals & Highlights

- **Standard C11 Architecture:** Pure C implementation without heavy C++ runtime dependencies, keeping manual memory management transparent and cache-friendly.
- **Compact Value Representation:** IEEE-754 64-bit NaN-boxing (`sizeof(Value) == 8`) packing numbers, booleans, `null`, and 48-bit heap pointers directly into 64-bit unsigned words.
- **Fast Bytecode VM:** Stack-based virtual machine with contiguous bytecode chunks, compact line checkpoint tables, and indexed constant pools.
- **Native JIT Engine:** Lightweight code generator designed to emit native machine code for critical execution paths.
- **Modular Library Layout:** Clean separation into standalone libraries (`ely_core`, `ely_compiler`, `ely_vm`, `ely_stdlib`, `ely_jit`).
- **First-Class Tooling:** CLI driver and unit test suite integrated with CMake.

---

## Workspace Architecture

The codebase is organized as a modular CMake workspace mirror to the C++ project:

```text
elysabettian-plainc/
├── CMakeLists.txt              # Root CMake configuration (C11, warnings, presets)
├── CMakePresets.json           # Presets for Debug and Release builds
├── libs/
│   ├── ely_core/               # Foundation: 64-bit NaN-boxed Value, Opcode ISA, Chunk, Debug utilities
│   │   ├── CMakeLists.txt
│   │   ├── include/ely/core/
│   │   └── src/
│   ├── ely_compiler/           # Lexer / Scanner, Pratt parser, and Bytecode compiler
│   │   ├── CMakeLists.txt
│   │   ├── include/ely/compiler/
│   │   └── src/
│   ├── ely_stdlib/             # Built-in standard library (math, io, system) and native registry
│   │   ├── CMakeLists.txt
│   │   ├── include/ely/stdlib/
│   │   └── src/
│   ├── ely_vm/                 # Stack VM runtime, heap objects, and execution engine
│   │   ├── CMakeLists.txt
│   │   ├── include/ely/vm/
│   │   └── src/
│   └── ely_jit/                # Native JIT compiler engine and runtime stubs
│       ├── CMakeLists.txt
│       ├── include/ely/jit/
│       └── src/
├── tools/
│   └── ely_cli/                # Command-line driver and interactive REPL
│       ├── CMakeLists.txt
│       ├── include/ely/cli/
│       └── src/
├── tests/                      # Unit test suites
│   ├── CMakeLists.txt
│   └── src/
├── examples/                   # Language benchmarks and example .ely scripts
└── docs/                       # Architecture specifications and memory layouts
```

---

## Building from Source

### Prerequisites
- **C Compiler:** GCC 11+, Clang 14+, or MSVC supporting **C11**.
- **CMake:** Version 3.25 or later.
- **Ninja** or **Make**.

### Build Commands

```bash
# Configure debug build using CMake presets
cmake --preset default

# Build all targets
cmake --build --preset default

# Or configure manually
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
