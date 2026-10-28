# Elysabettian (Modern C++ Implementation)

A high-performance runtime, bytecode virtual machine, and native JIT compiler architecture for the **Elysabettian** scripting language, designed in modern **C++20**.

---

## Overview

Elysabettian is a lightweight, dynamically typed scripting language featuring clean, JavaScript-like syntax. This repository represents the modern C++ implementation of the runtime—re-architecting the modular design and lessons learned from the Rust port (`elysabettian-rs`) back into idiomatic, zero-overhead C++20.

### Key Goals & Highlights

- **Modern C++20 Architecture:** Built with `std::string_view`, `std::span`, concepts, `std::bit_cast`, `std::optional`, and `std::ranges` for zero-allocation parsing, type safety, and maximum CPU cache locality.
- **Compact Value Representation:** IEEE-754 64-bit NaN-boxing (`sizeof(Value) == 8`) packing numbers, booleans, `null`, and 48-bit heap pointers directly into registers.
- **Fast Bytecode VM:** Stack-based virtual machine with contiguous bytecode chunks, compact line checkpoint tables with logarithmic resolution via binary search (`std::ranges::lower_bound`), and indexed constant pools.
- **Native JIT Engine (AsmJit):** Designed for low-latency native machine code emission (x86_64 and AArch64) using [AsmJit](https://asmjit.com/)—replacing Rust's Cranelift dependency with an ultra-lightweight, native C++ JIT engine without external compiler overhead.
- **First-Class Tooling:** Built-in disassembler (`ely::core::disassemble_chunk`), CLI driver, and comprehensive test suite.

---

## Workspace Architecture

The codebase is organized as a modular CMake workspace:

```text
elysabettian-cpp/
├── CMakeLists.txt              # Root CMake configuration (C++20, warnings, presets)
├── CMakePresets.json           # Presets for Debug and Release builds
├── libs/
│   ├── ely_core/               # Foundation: 64-bit NaN-boxed Value, Opcode ISA, Chunk, Disassembler
│   ├── ely_compiler/           # Zero-allocation Lexer, Pratt parser, and Bytecode emitter
│   ├── ely_stdlib/             # Built-in standard library (math, io) and dynamic C-ABI loader
│   └── ely_vm/                 # Stack VM runtime, memory heap, and execution engine
├── tools/
│   └── ely_cli/                # Command-line driver and interactive REPL
├── tests/                      # Unit test suites
├── examples/                   # Language benchmarks and example .ely scripts
└── docs/                       # Architecture specifications and memory layouts
```

---

## Architectural Evolution: Rust vs C++20

| Feature / Subsystem | Rust (`elysabettian-rs`) | Modern C++ (`elysabettian-cpp`) |
| :--- | :--- | :--- |
| **String / Slices** | `&str`, `&[T]` | `std::string_view`, `std::span<const T>` |
| **Bit Casting** | `f64::to_bits`, `f64::from_bits` | `std::bit_cast<uint64_t>(double)` (C++20) |
| **Binary Search** | `lines.binary_search_by(...)` | `std::ranges::lower_bound(m_lines, ..., &LineInfo::offset)` |
| **Recursive Types** | `Box<Chunk>` in `FunctionDescriptor` | `std::unique_ptr<Chunk>` with explicit move-only semantics |
| **JIT Engine** | Cranelift (heavy SSA compiler) | **AsmJit** (lightweight, zero-dependency C++ native JIT) |
| **String Formatting**| `format!("{:#x}", ...)` | `std::format("<object {:#014x}>", ...)` (C++20) |
| **Jump Offsets** | `offset as isize + sign * jump` | `static_cast<ptrdiff_t>(offset) + sign * jump` |

---

## Documentation Directory

Full engineering specifications and architectural guides are available in [`docs/`](docs/):

| Document | Description |
| :--- | :--- |
| [System Architecture](docs/architecture.md) | End-to-end design, modular architecture, and execution pipeline. |
| [AsmJit Compiler Specification](docs/jit_compiler.md) | Native JIT engine architecture, register allocation, and machine emission via AsmJit. |
| [Bytecode & Opcode Reference](docs/opcodes_specification.md) | ISA definition, operand encodings, and stack transition rules. |
| [NaN-Boxing Value Specification](docs/nan_boxing_specification.md) | IEEE-754 64-bit layout, quiet NaNs, and payload bitmasks. |
| [CLI & Tooling Guide](docs/cli_tooling.md) | Driver flags, interactive REPL, and benchmark runner. |

---

## Building from Source

### Prerequisites
- **C++ Compiler:** GCC 11+, Clang 14+, or MSVC 2019+ supporting **C++20** (`<bit>`, `std::format`, `std::ranges`, `std::span`).
- **CMake:** Version 3.25 or later.
- **Ninja** or **Make** (recommended).

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
