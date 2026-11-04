#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <ely/core/chunk.hpp>
#include <ely/core/opcode.hpp>
#include <ely/core/value.hpp>
#include <ely/jit/compiler.hpp>
#include <ely/jit/engine.hpp>
#include <ely/jit/runtime.hpp>
#include <ely/compiler/compiler.hpp>
#include <iostream>
#include <sstream>

using ely::core::Chunk;
using ely::core::Opcode;
using ely::core::Value;
using ely::jit::JitCompiler;
using ely::jit::JitEngine;

TEST_CASE("JIT arithmetic expression execution", "[jit][arithmetic]") {
    JitEngine engine;
    Chunk chunk;

    // Bytecode for: (15.5 + 4.5) * 2.0 - 10.0 / 2.0 = 20.0 * 2.0 - 5.0 = 35.0
    const auto c1 = chunk.add_constant(Value::number(15.5));
    const auto c2 = chunk.add_constant(Value::number(4.5));
    const auto c3 = chunk.add_constant(Value::number(2.0));
    const auto c4 = chunk.add_constant(Value::number(10.0));
    const auto c5 = chunk.add_constant(Value::number(2.0));

    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c1), 1);
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c2), 1);
    chunk.write_opcode(Opcode::Add, 1);

    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c3), 1);
    chunk.write_opcode(Opcode::Multiply, 1);

    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c4), 1);
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c5), 1);
    chunk.write_opcode(Opcode::Divide, 1);

    chunk.write_opcode(Opcode::Subtract, 1);
    chunk.write_opcode(Opcode::Return, 1);

    JitCompiler compiler(engine);
    const auto res = compiler.compile_arithmetic(chunk, "test_math");
    REQUIRE(res);
    REQUIRE(res.code_ptr != nullptr);

    using Fn = double (*)();
    auto fn = reinterpret_cast<Fn>(const_cast<uint8_t*>(res.code_ptr));
    const double result = fn();
    REQUIRE_THAT(result, Catch::Matchers::WithinRel(35.0, 1e-9));
}

TEST_CASE("JIT function with parameters and local variables", "[jit][params]") {
    JitEngine engine;
    Chunk chunk;

    // Computes: (a * 2.0) + (b - 3.0)
    // Local slots: slot 1 = a, slot 2 = b, slot 3 = c
    const auto c_two = chunk.add_constant(Value::number(2.0));
    const auto c_three = chunk.add_constant(Value::number(3.0));

    // c = a * 2.0
    chunk.write_opcode(Opcode::GetLocal, 1);
    chunk.write_byte(1, 1); // a
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_two), 1);
    chunk.write_opcode(Opcode::Multiply, 1);
    chunk.write_opcode(Opcode::SetLocal, 1);
    chunk.write_byte(3, 1); // c
    chunk.write_opcode(Opcode::Pop, 1);

    // c + (b - 3.0)
    chunk.write_opcode(Opcode::GetLocal, 1);
    chunk.write_byte(3, 1); // c
    chunk.write_opcode(Opcode::GetLocal, 1);
    chunk.write_byte(2, 1); // b
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_three), 1);
    chunk.write_opcode(Opcode::Subtract, 1);
    chunk.write_opcode(Opcode::Add, 1);
    chunk.write_opcode(Opcode::Return, 1);

    JitCompiler compiler(engine);
    const auto res = compiler.compile_function(chunk, 2, "test_params_locals");
    REQUIRE(res);

    using Fn = double (*)(double, double);
    auto fn = reinterpret_cast<Fn>(const_cast<uint8_t*>(res.code_ptr));
    const double result = fn(10.0, 8.0);
    // (10.0 * 2.0) + (8.0 - 3.0) = 20.0 + 5.0 = 25.0
    REQUIRE_THAT(result, Catch::Matchers::WithinRel(25.0, 1e-9));
}

TEST_CASE("JIT bitwise operations execution", "[jit][bitwise]") {
    JitEngine engine;
    Chunk chunk;

    const auto c_a = chunk.add_constant(Value::number(12.0)); // 0b1100
    const auto c_b = chunk.add_constant(Value::number(10.0)); // 0b1010
    const auto c_shift = chunk.add_constant(Value::number(2.0));

    // (12 & 10) -> 8
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_a), 1);
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_b), 1);
    chunk.write_opcode(Opcode::BwAnd, 1);

    // (8 << 2) -> 32
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_shift), 1);
    chunk.write_opcode(Opcode::ShiftLeft, 1);

    chunk.write_opcode(Opcode::Return, 1);

    JitCompiler compiler(engine);
    const auto res = compiler.compile_function(chunk, 0, "test_bitwise");
    REQUIRE(res);

    using Fn = double (*)();
    auto fn = reinterpret_cast<Fn>(const_cast<uint8_t*>(res.code_ptr));
    REQUIRE_THAT(fn(), Catch::Matchers::WithinRel(32.0, 1e-9));
}

TEST_CASE("JIT dead code elimination after return", "[jit][dead_code]") {
    JitEngine engine;
    Chunk chunk;

    const auto c_val = chunk.add_constant(Value::number(42.0));
    const auto c_dead = chunk.add_constant(Value::number(999.0));

    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_val), 1);
    chunk.write_opcode(Opcode::Return, 1);

    // Dead code after return: should be safely skipped
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_dead), 1);
    chunk.write_opcode(Opcode::Nop, 1);
    chunk.write_opcode(Opcode::Return, 1);

    JitCompiler compiler(engine);
    const auto res = compiler.compile_function(chunk, 0, "test_dead_code");
    REQUIRE(res);

    using Fn = double (*)();
    auto fn = reinterpret_cast<Fn>(const_cast<uint8_t*>(res.code_ptr));
    REQUIRE_THAT(fn(), Catch::Matchers::WithinRel(42.0, 1e-9));
}

TEST_CASE("JIT loop and conditional branching", "[jit][loop]") {
    JitEngine engine;
    Chunk chunk;

    // Computes sum of integers from 1 to 5:
    // Slot 1: sum = 0.0
    // Slot 2: i = 1.0
    // Slot 3: limit = 5.0
    const auto c_zero = chunk.add_constant(Value::number(0.0));
    const auto c_one = chunk.add_constant(Value::number(1.0));
    const auto c_five = chunk.add_constant(Value::number(5.0));

    // sum = 0.0
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_zero), 1);
    chunk.write_opcode(Opcode::SetLocal, 1);
    chunk.write_byte(1, 1);
    chunk.write_opcode(Opcode::Pop, 1);

    // i = 1.0
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_one), 1);
    chunk.write_opcode(Opcode::SetLocal, 1);
    chunk.write_byte(2, 1);
    chunk.write_opcode(Opcode::Pop, 1);

    // limit = 5.0
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_five), 1);
    chunk.write_opcode(Opcode::SetLocal, 1);
    chunk.write_byte(3, 1);
    chunk.write_opcode(Opcode::Pop, 1);

    // Loop header (IP offset for loop start)
    const size_t loop_start_ip = chunk.code().size();

    // Check condition: i <= limit <=> (i > limit) == false
    // greater: i > limit
    chunk.write_opcode(Opcode::GetLocal, 1);
    chunk.write_byte(2, 1); // i
    chunk.write_opcode(Opcode::GetLocal, 1);
    chunk.write_byte(3, 1); // limit
    chunk.write_opcode(Opcode::Greater, 1);

    // Jump if (i > limit) is true => JumpZero if !(i > limit) falls through to body
    // In our JIT: JumpZero jumps if condition is false/zero (i.e. i <= limit)
    // So JumpZero to body_label!
    const auto jump_to_body = chunk.code().size();
    chunk.write_opcode(Opcode::JumpZero, 1);
    chunk.write_byte(0xFF, 1);
    chunk.write_byte(0xFF, 1);
    chunk.write_opcode(Opcode::Pop, 1); // pop condition on fallthrough

    // If condition was true (i > limit), break out of loop:
    const auto jump_to_exit = chunk.code().size();
    chunk.write_opcode(Opcode::Jump, 1);
    chunk.write_byte(0xFF, 1);
    chunk.write_byte(0xFF, 1);

    // Body starts here:
    const size_t body_ip = chunk.code().size();
    // Patch jump_to_body:
    const uint16_t body_offset = static_cast<uint16_t>(body_ip - (jump_to_body + 3));
    chunk.patch_byte(jump_to_body + 1, static_cast<uint8_t>((body_offset >> 8) & 0xFF));
    chunk.patch_byte(jump_to_body + 2, static_cast<uint8_t>(body_offset & 0xFF));

    chunk.write_opcode(Opcode::Pop, 1); // pop condition

    // sum = sum + i
    chunk.write_opcode(Opcode::GetLocal, 1);
    chunk.write_byte(1, 1); // sum
    chunk.write_opcode(Opcode::GetLocal, 1);
    chunk.write_byte(2, 1); // i
    chunk.write_opcode(Opcode::Add, 1);
    chunk.write_opcode(Opcode::SetLocal, 1);
    chunk.write_byte(1, 1); // sum
    chunk.write_opcode(Opcode::Pop, 1);

    // i = i + 1.0
    chunk.write_opcode(Opcode::GetLocal, 1);
    chunk.write_byte(2, 1); // i
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_one), 1);
    chunk.write_opcode(Opcode::Add, 1);
    chunk.write_opcode(Opcode::SetLocal, 1);
    chunk.write_byte(2, 1); // i
    chunk.write_opcode(Opcode::Pop, 1);

    // Loop back:
    const size_t loop_back_ip = chunk.code().size() + 3;
    const uint16_t loop_offset = static_cast<uint16_t>(loop_back_ip - loop_start_ip);
    chunk.write_opcode(Opcode::Loop, 1);
    chunk.write_byte(static_cast<uint8_t>((loop_offset >> 8) & 0xFF), 1);
    chunk.write_byte(static_cast<uint8_t>(loop_offset & 0xFF), 1);

    // Exit starts here:
    const size_t exit_ip = chunk.code().size();
    const uint16_t exit_offset = static_cast<uint16_t>(exit_ip - (jump_to_exit + 3));
    chunk.patch_byte(jump_to_exit + 1, static_cast<uint8_t>((exit_offset >> 8) & 0xFF));
    chunk.patch_byte(jump_to_exit + 2, static_cast<uint8_t>(exit_offset & 0xFF));

    // Return sum
    chunk.write_opcode(Opcode::GetLocal, 1);
    chunk.write_byte(1, 1);
    chunk.write_opcode(Opcode::Return, 1);

    JitCompiler compiler(engine);
    const auto res = compiler.compile_function(chunk, 0, "test_loop");
    REQUIRE(res);

    using Fn = double (*)();
    auto fn = reinterpret_cast<Fn>(const_cast<uint8_t*>(res.code_ptr));
    const double val = fn();
    REQUIRE_THAT(val, Catch::Matchers::WithinRel(15.0, 1e-9));
}

TEST_CASE("JIT globals and runtime integration", "[jit][globals]") {
    ely::jit::reset_jit_runtime();

    JitEngine engine;
    Chunk chunk;

    const auto c_val = chunk.add_constant(Value::number(123.456));
    const auto c_name = chunk.add_constant(Value::obj(42)); // name_id = 42

    // DefineGlobal 42 = 123.456
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c_val), 1);
    chunk.write_opcode(Opcode::DefineGlobal, 1);
    chunk.write_byte(static_cast<uint8_t>(c_name), 1);

    // GetGlobal 42
    chunk.write_opcode(Opcode::GetGlobal, 1);
    chunk.write_byte(static_cast<uint8_t>(c_name), 1);

    // Return it
    chunk.write_opcode(Opcode::Return, 1);

    JitCompiler compiler(engine);
    const auto res = compiler.compile_function(chunk, 0, "test_globals");
    REQUIRE(res);

    using Fn = double (*)();
    auto fn = reinterpret_cast<Fn>(const_cast<uint8_t*>(res.code_ptr));
    REQUIRE_THAT(fn(), Catch::Matchers::WithinRel(123.456, 1e-9));

    // Verify global state in runtime
    const auto global_bits = ely_jit_get_global(42);
    const auto global_val = Value::from_raw(global_bits);
    REQUIRE(global_val.is_number());
    REQUIRE_THAT(global_val.as_number(), Catch::Matchers::WithinRel(123.456, 1e-9));
}

TEST_CASE("JIT dump_ir and dump_asm output", "[jit][dump]") {
    JitEngine engine;
    Chunk chunk;

    const auto c1 = chunk.add_constant(Value::number(10.0));
    const auto c2 = chunk.add_constant(Value::number(20.0));
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c1), 1);
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c2), 1);
    chunk.write_opcode(Opcode::Add, 1);
    chunk.write_opcode(Opcode::Return, 1);

    JitCompiler compiler(engine);
    compiler.dump_ir = true;
    compiler.dump_asm = true;

    // Redirect stdout to inspect dumped IR and ASM
    std::stringstream buffer;
    std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());

    const auto res = compiler.compile_arithmetic(chunk, "inspect_dump");

    std::cout.rdbuf(old_cout);

    REQUIRE(res);
    REQUIRE(res.code_ptr != nullptr);

    const std::string output = buffer.str();
    CHECK(output.find("=== AsmJit IR [inspect_dump] ===") != std::string::npos);
    CHECK(output.find("=== Machine Code Disassembly [inspect_dump] ===") != std::string::npos);
}

TEST_CASE("JIT ARM64 (AArch64) code emission and disassembly", "[jit][a64]") {
    JitEngine engine;
    Chunk chunk;

    const auto c1 = chunk.add_constant(Value::number(40.0));
    const auto c2 = chunk.add_constant(Value::number(2.0));
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c1), 1);
    chunk.write_opcode(Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(c2), 1);
    chunk.write_opcode(Opcode::Add, 1);
    chunk.write_opcode(Opcode::Return, 1);

    JitCompiler compiler(engine);
    compiler.dump_ir = true;
    compiler.dump_asm = true;

    std::stringstream buffer;
    std::streambuf* old_cout = std::cout.rdbuf(buffer.rdbuf());

    const auto res = compiler.compile_function_a64(chunk, 0, "arm64_test");

    std::cout.rdbuf(old_cout);

    if (!res.error.empty()) {
        std::cerr << "compile_function_a64 error: " << res.error << "\n";
    }

    const std::string output = buffer.str();
    CHECK(output.find("=== AsmJit IR [arm64_test] ===") != std::string::npos);
    CHECK(output.find("=== Machine Code Disassembly [arm64_test] ===") != std::string::npos);
    // Verifies ARM64 instruction (fadd) was emitted
    CHECK(output.find("fadd") != std::string::npos);
}

TEST_CASE("JIT compiled factorial with for-loop and local variables", "[jit][factorial]") {
    ely::compiler::Compiler comp("func factorial(x) { var f = 1; for (var i = 1; i <= x; ++i) { f *= i; } return f; }");
    auto chunk = comp.compile();
    REQUIRE(chunk.has_value());
    REQUIRE(!chunk->function_constants().empty());

    const auto& func_desc = chunk->function_constants()[0];
    REQUIRE(func_desc.name == "factorial");
    REQUIRE(func_desc.arity == 1);
    REQUIRE(func_desc.chunk != nullptr);

    JitEngine engine;
    JitCompiler compiler(engine);
    const auto res = compiler.compile_function(*func_desc.chunk, func_desc.arity, "factorial");
    REQUIRE(res);
    REQUIRE(res.code_ptr != nullptr);

    using Fn = double (*)(double);
    auto fn = reinterpret_cast<Fn>(const_cast<uint8_t*>(res.code_ptr));
    REQUIRE_THAT(fn(5.0), Catch::Matchers::WithinRel(120.0, 1e-9));
    REQUIRE_THAT(fn(6.0), Catch::Matchers::WithinRel(720.0, 1e-9));
}
