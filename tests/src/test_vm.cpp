#include <catch2/catch_test_macros.hpp>
#include <ely/compiler/compiler.hpp>
#include <ely/core/chunk.hpp>
#include <ely/core/opcode.hpp>
#include <ely/core/value.hpp>
#include <ely/vm/vm.hpp>

static void emit_constant(ely::core::Chunk& chunk, ely::core::Value val, uint32_t line = 1) {
    const auto idx = chunk.add_constant(val);
    chunk.write_opcode(ely::core::Opcode::Constant, line);
    chunk.write_byte(static_cast<uint8_t>(idx), line);
}

TEST_CASE("VM interpret simple return", "[vm]") {
    ely::vm::VM vm;
    ely::core::Chunk chunk;

    emit_constant(chunk, ely::core::Value::number(42.0));
    chunk.write_opcode(ely::core::Opcode::Return, 1);

    const auto result = vm.interpret(chunk);
    REQUIRE(result == ely::vm::InterpretResult::Ok);
    REQUIRE(vm.peek(0) == ely::core::Value::number(42.0));
}

TEST_CASE("VM interpret binary arithmetic", "[vm]") {
    ely::vm::VM vm;
    ely::core::Chunk chunk;

    emit_constant(chunk, ely::core::Value::number(10.0));
    emit_constant(chunk, ely::core::Value::number(5.0));
    chunk.write_opcode(ely::core::Opcode::Add, 1);
    emit_constant(chunk, ely::core::Value::number(2.0));
    chunk.write_opcode(ely::core::Opcode::Multiply, 1);
    chunk.write_opcode(ely::core::Opcode::Return, 1);

    const auto result = vm.interpret(chunk);
    REQUIRE(result == ely::vm::InterpretResult::Ok);
    REQUIRE(vm.peek(0) == ely::core::Value::number(30.0));
}

TEST_CASE("VM interpret bitwise operations", "[vm]") {
    ely::vm::VM vm;
    ely::core::Chunk chunk;

    emit_constant(chunk, ely::core::Value::number(12.0));
    emit_constant(chunk, ely::core::Value::number(5.0));
    chunk.write_opcode(ely::core::Opcode::BwAnd, 1);
    emit_constant(chunk, ely::core::Value::number(3.0));
    chunk.write_opcode(ely::core::Opcode::BwOr, 1);
    chunk.write_opcode(ely::core::Opcode::Return, 1);

    const auto result = vm.interpret(chunk);
    REQUIRE(result == ely::vm::InterpretResult::Ok);
    REQUIRE(vm.peek(0) == ely::core::Value::number(7.0));
}

TEST_CASE("VM interpret array operations", "[vm]") {
    ely::vm::VM vm;
    ely::core::Chunk chunk;

    emit_constant(chunk, ely::core::Value::number(10.0));
    emit_constant(chunk, ely::core::Value::number(20.0));
    emit_constant(chunk, ely::core::Value::number(30.0));
    chunk.write_opcode(ely::core::Opcode::ArrayBuild, 1);
    chunk.write_byte(3, 1);

    emit_constant(chunk, ely::core::Value::number(1.0));
    chunk.write_opcode(ely::core::Opcode::ArrayIndex, 1);

    chunk.write_opcode(ely::core::Opcode::Return, 1);

    const auto result = vm.interpret(chunk);
    REQUIRE(result == ely::vm::InterpretResult::Ok);
    REQUIRE(vm.peek(0) == ely::core::Value::number(20.0));
}

TEST_CASE("VM interpret class instance and properties", "[vm]") {
    ely::vm::VM vm;
    ely::core::Chunk chunk;

    // 1. Declare class
    const auto class_name_const = chunk.add_constant(ely::core::Value::obj(1));
    chunk.write_opcode(ely::core::Opcode::Class, 1);
    chunk.write_byte(static_cast<uint8_t>(class_name_const), 1);

    // 2. Instantiate class: Call with 0 args
    chunk.write_opcode(ely::core::Opcode::Call, 1);
    chunk.write_byte(0, 1);

    // Store instance in local slot 0
    chunk.write_opcode(ely::core::Opcode::SetLocal, 1);
    chunk.write_byte(0, 1);

    // Set property field 42 = 999.0: pushes instance, pushes value
    chunk.write_opcode(ely::core::Opcode::GetLocal, 1);
    chunk.write_byte(0, 1);
    const auto field_name_const = chunk.add_constant(ely::core::Value::obj(42));
    emit_constant(chunk, ely::core::Value::number(999.0));
    chunk.write_opcode(ely::core::Opcode::SetProperty, 1);
    chunk.write_byte(static_cast<uint8_t>(field_name_const), 1);
    // Pop result of SetProperty
    chunk.write_opcode(ely::core::Opcode::Pop, 1);

    // Get property field 42: push instance, GetProperty
    chunk.write_opcode(ely::core::Opcode::GetLocal, 1);
    chunk.write_byte(0, 1);
    chunk.write_opcode(ely::core::Opcode::GetProperty, 1);
    chunk.write_byte(static_cast<uint8_t>(field_name_const), 1);

    chunk.write_opcode(ely::core::Opcode::Return, 1);

    const auto result = vm.interpret(chunk);
    REQUIRE(result == ely::vm::InterpretResult::Ok);
    REQUIRE(vm.peek(0) == ely::core::Value::number(999.0));
}

TEST_CASE("VM interpret inherit and invoke", "[vm]") {
    ely::vm::VM vm;

    // Build method function: returns 42.0 (takes 0 args, slot 0 is receiver)
    auto method_chunk = std::make_shared<ely::core::Chunk>();
    emit_constant(*method_chunk, ely::core::Value::number(42.0));
    method_chunk->write_opcode(ely::core::Opcode::Return, 1);

    ely::vm::ObjFunction method_func(0, "foo");
    method_func.chunk = method_chunk;
    const auto method_func_val = vm.allocate_object(std::move(method_func));

    ely::core::Chunk chunk;
    // 1. Superclass A
    const auto class_a_name = chunk.add_constant(ely::core::Value::obj(1));
    chunk.write_opcode(ely::core::Opcode::Class, 1);
    chunk.write_byte(static_cast<uint8_t>(class_a_name), 1);

    // Add method 'foo' (name id 100) to class A
    const auto method_name_const = chunk.add_constant(ely::core::Value::obj(100));
    const auto method_const = chunk.add_constant(method_func_val);
    chunk.write_opcode(ely::core::Opcode::Constant, 1);
    chunk.write_byte(static_cast<uint8_t>(method_const), 1);
    chunk.write_opcode(ely::core::Opcode::Method, 1);
    chunk.write_byte(static_cast<uint8_t>(method_name_const), 1);

    // 2. Subclass B
    const auto class_b_name = chunk.add_constant(ely::core::Value::obj(2));
    chunk.write_opcode(ely::core::Opcode::Class, 1);
    chunk.write_byte(static_cast<uint8_t>(class_b_name), 1);

    // Inherit: stack has [Class A, Class B]
    chunk.write_opcode(ely::core::Opcode::Inherit, 1);
    // Inherit pops Class B, Class A is left on stack. Pop Class A
    chunk.write_opcode(ely::core::Opcode::Pop, 1);

    // Instantiate Class B (which is on heap id 2)
    // Push Class B directly and Call(0)
    chunk.write_opcode(ely::core::Opcode::Constant, 1);
    const auto class_b_val = ely::core::Value::obj(2);
    const auto b_const = chunk.add_constant(class_b_val);
    chunk.write_byte(static_cast<uint8_t>(b_const), 1);
    chunk.write_opcode(ely::core::Opcode::Call, 1);
    chunk.write_byte(0, 1);

    // Stack has Instance of B. Invoke 'foo' with 0 args
    chunk.write_opcode(ely::core::Opcode::Invoke, 1);
    chunk.write_byte(static_cast<uint8_t>(method_name_const), 1);
    chunk.write_byte(0, 1);

    // Return result
    chunk.write_opcode(ely::core::Opcode::Return, 1);

    const auto result = vm.interpret(chunk);
    REQUIRE(result == ely::vm::InterpretResult::Ok);
    REQUIRE(vm.peek(0) == ely::core::Value::number(42.0));
}

TEST_CASE("VM interpret prefix inc/dec and compound assignments", "[vm]") {
    const std::string_view source = R"(
        func compute() {
            var x = 10;
            ++x;       // 11
            x += 5;    // 16
            --x;       // 15
            x -= 3;    // 12
            x *= 2;    // 24
            x /= 4;    // 6
            return x;
        }
        compute();
    )";

    ely::vm::VM vm;
    const auto result = vm.interpret(source);
    REQUIRE(result == ely::vm::InterpretResult::Ok);
}

TEST_CASE("VM interpret postfix increment/decrement", "[vm]") {
    const std::string_view source = R"(
        func compute() {
            var x = 10;
            var old_x = x++;
            var old_x2 = x--;
            var y = x++ * 2;
            return old_x + old_x2 + y + x;
        }
        compute();
    )";

    ely::vm::VM vm;
    const auto result = vm.interpret(source);
    REQUIRE(result == ely::vm::InterpretResult::Ok);
}

TEST_CASE("VM interpret string concatenation", "[vm]") {
    ely::vm::VM vm;
    ely::core::Chunk chunk;

    const auto h_idx = chunk.add_string_constant("Hello, ");
    const auto w_idx = chunk.add_string_constant("world!");

    chunk.write_opcode(ely::core::Opcode::Constant, 1);
    const auto c1 = chunk.add_constant(ely::core::Value::obj(h_idx));
    chunk.write_byte(static_cast<uint8_t>(c1), 1);

    chunk.write_opcode(ely::core::Opcode::Constant, 1);
    const auto c2 = chunk.add_constant(ely::core::Value::obj(w_idx));
    chunk.write_byte(static_cast<uint8_t>(c2), 1);

    chunk.write_opcode(ely::core::Opcode::Add, 1);
    chunk.write_opcode(ely::core::Opcode::Return, 1);

    const auto result = vm.interpret(chunk);
    REQUIRE(result == ely::vm::InterpretResult::Ok);
    REQUIRE(vm.format_value(vm.peek(0)) == "Hello, world!");
}

TEST_CASE("VM interpret multiple string constants across chunks", "[vm]") {
    ely::vm::VM vm;
    ely::compiler::Compiler comp1("var s1 = 'cc';");
    auto chunk1 = comp1.compile();
    REQUIRE(chunk1.has_value());
    const auto res1 = vm.interpret(*chunk1);
    REQUIRE(res1 == ely::vm::InterpretResult::Ok);

    ely::compiler::Compiler comp2("var s2 = 'ciao';");
    auto chunk2 = comp2.compile();
    REQUIRE(chunk2.has_value());
    const auto res2 = vm.interpret(*chunk2);
    REQUIRE(res2 == ely::vm::InterpretResult::Ok);
}

TEST_CASE("VM interpret closure and upvalues", "[vm]") {
    ely::vm::VM vm;

    // Inner function: takes 0 args, reads upvalue 0, returns it
    auto inner_chunk = std::make_shared<ely::core::Chunk>();
    inner_chunk->write_opcode(ely::core::Opcode::GetUpvalue, 1);
    inner_chunk->write_byte(0, 1); // upvalue index 0
    inner_chunk->write_opcode(ely::core::Opcode::Return, 1);

    ely::vm::ObjFunction inner_func(0, "inner");
    inner_func.upvalue_count = 1;
    inner_func.chunk = inner_chunk;
    const auto inner_func_val = vm.allocate_object(std::move(inner_func));

    // Outer chunk: local slot 1 has 99.0, creates closure capturing slot 1, calls closure
    ely::core::Chunk chunk;
    emit_constant(chunk, ely::core::Value::number(99.0)); // slot 1
    const auto c_inner = chunk.add_constant(inner_func_val);
    chunk.write_opcode(ely::core::Opcode::Closure, 1);
    chunk.write_byte(static_cast<uint8_t>(c_inner), 1);
    chunk.write_byte(1, 1); // is_local = 1
    chunk.write_byte(1, 1); // index = 1 (slot 1)

    // Call inner closure (0 args)
    chunk.write_opcode(ely::core::Opcode::Call, 1);
    chunk.write_byte(0, 1);

    chunk.write_opcode(ely::core::Opcode::Return, 1);

    const auto result = vm.interpret(chunk);
    REQUIRE(result == ely::vm::InterpretResult::Ok);
    REQUIRE(vm.peek(0) == ely::core::Value::number(99.0));
}

