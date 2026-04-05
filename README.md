# Elysabettian

> A lightweight, dynamically-typed scripting language with a bytecode virtual machine — written in Rust.

Elysabettian is a scripting language designed for simplicity and clarity. It features a JavaScript-inspired syntax, first-class functions, closures, and a class system with single inheritance. The runtime is built around a register-based bytecode VM compiled from a typed AST, with rich diagnostic error reporting.

This is a from-scratch Rust rewrite of the [original C++17 implementation](https://github.com/quark/elysabettian), with a focus on correctness, performance, and maintainability.

---

## Features

- **Dynamic typing** — numbers, booleans, strings, null, arrays, and class instances
- **First-class functions** — closures with lexical scoping and upvalue capture
- **Classes** — single inheritance, instance fields, and constructors via `init`
- **Bytecode VM** — sources compile to a typed chunk of bytecode, executed by a fast VM
- **Rich error diagnostics** — precise source spans with annotated error messages
- **Standard library** — math, I/O, and array utilities built in
- **REPL and file execution** — interactive prompt or run `.ely` scripts directly

---

## Language Overview

```js
// Functions and recursion
func fib(n) {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
}

print fib(10); // 55
```

```js
// Closures
func make_counter() {
  var count = 0;
  func increment() {
    count = count + 1;
    return count;
  }
  return increment;
}

var counter = make_counter();
print counter(); // 1
print counter(); // 2
```

```js
// Classes and inheritance
class Shape {
  init(color) {
    this.color = color;
  }

  describe() {
    print "A " + this.color + " shape.";
  }
}

class Circle < Shape {
  init(color, radius) {
    super.init(color);
    this.radius = radius;
  }

  area() {
    var math = import("math");
    return math.PI * this.radius * this.radius;
  }
}

var c = Circle("red", 5);
c.describe();           // A red shape.
print c.area();         // 78.539...
```

```js
// Arrays
var nums = [1, 2, 3, 4, 5];
push(nums, 6);
print len(nums); // 6
print nums[0];   // 1
```

---

## Getting Started

### Prerequisites

- [Rust](https://www.rust-lang.org/tools/install) 1.85 or newer (edition 2024)

### Build

```bash
git clone https://github.com/quark/elysabettian
cd elysabettian
cargo build --release
```

The compiled binary will be at `target/release/elysabettian`.

### Run

**Interactive REPL:**
```bash
cargo run
```

**Execute a script:**
```bash
cargo run -- script.ely
```

**Evaluate an expression:**
```bash
cargo run -- -c "print 1 + 2;"
```

---

## Project Structure

```
src/
├── main.rs          # CLI entry point and REPL
├── lib.rs           # Public API
├── error.rs         # Unified diagnostic error types
├── lexer/
│   ├── mod.rs       # Tokenizer
│   └── token.rs     # Token definitions
├── parser/
│   ├── mod.rs       # Recursive descent / Pratt parser
│   └── ast.rs       # Typed AST node definitions
├── compiler/
│   ├── mod.rs       # AST → bytecode compiler
│   └── chunk.rs     # Bytecode chunk and constant pool
├── vm/
│   ├── mod.rs       # Bytecode virtual machine
│   └── value.rs     # Runtime value representation
└── stdlib/
    └── mod.rs       # Built-in functions and modules
```

---

## Architecture

Elysabettian uses a classic multi-stage pipeline:

```
Source
  │
  ▼
Lexer          tokenizer.rs    →  Token stream
  │
  ▼
Parser         parser/         →  Typed AST
  │
  ▼
Compiler       compiler/       →  Bytecode (Chunk)
  │
  ▼
VM             vm/             →  Execution
```

The AST is a full intermediate representation, which allows the compiler to perform constant folding and other simple optimizations before emitting bytecode. The VM is register-based, reducing instruction count compared to a purely stack-based design.

---

## Standard Library

Modules are loaded on demand with the built-in `import` function:

```js
var math = import("math");
print math.sqrt(2); // 1.4142...
```

| Module   | Functions |
|----------|-----------|
| `math`   | `sin`, `cos`, `tan`, `sqrt`, `pow`, `abs`, `floor`, `ceil`, `log`, `exp`, `min`, `max`, `random`, `PI` |
| `stdio`  | `read`, `write`, `readFile`, `writeFile`, `openFile`, `closeFile` |
| `array`  | `push`, `pop`, `len` (also available as globals) |

Built-in globals: `clock()`, `date()`, `string()`, `version()`, `exit()`.

---

## License

MIT

---

*Elysabettian was originally developed as a Bachelor of Science thesis project in Computer Science.*
