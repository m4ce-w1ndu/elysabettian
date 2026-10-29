#include <catch2/catch_test_macros.hpp>
#include <ely/compiler/compiler.hpp>
#include <ely/core/opcode.hpp>
#include <vector>

static std::vector<ely::core::Opcode> extract_opcodes(const ely::core::Chunk& chunk) {
    std::vector<ely::core::Opcode> ops;
    size_t i = 0;
    while (i < chunk.size()) {
        const auto byte = chunk.code()[i];
        const auto op = static_cast<ely::core::Opcode>(byte);
        ops.push_back(op);

        switch (op) {
            case ely::core::Opcode::Constant:
            case ely::core::Opcode::GetLocal:
            case ely::core::Opcode::SetLocal:
            case ely::core::Opcode::GetGlobal:
            case ely::core::Opcode::DefineGlobal:
            case ely::core::Opcode::SetGlobal:
            case ely::core::Opcode::Call:
            case ely::core::Opcode::Closure:
            case ely::core::Opcode::Import:
                i += 2;
                break;
            case ely::core::Opcode::Jump:
            case ely::core::Opcode::JumpZero:
            case ely::core::Opcode::Loop:
                i += 3;
                break;
            default:
                i += 1;
                break;
        }
    }
    return ops;
}

TEST_CASE("Compiler arithmetic expression", "[compiler]") {
    ely::compiler::Compiler comp("1 + 2 * 3;");
    auto chunk = comp.compile();
    REQUIRE(chunk.has_value());

    const auto ops = extract_opcodes(*chunk);
    const std::vector<ely::core::Opcode> expected = {
        ely::core::Opcode::Constant,
        ely::core::Opcode::Constant,
        ely::core::Opcode::Constant,
        ely::core::Opcode::Multiply,
        ely::core::Opcode::Add,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Nop,
        ely::core::Opcode::Return
    };

    REQUIRE(ops == expected);
}

TEST_CASE("Compiler global variable declaration and usage", "[compiler]") {
    std::string_view src = 
        "var a = 10;\n"
        "var b = 20;\n"
        "print a + b;\n";

    ely::compiler::Compiler comp(src);
    auto chunk = comp.compile();
    REQUIRE(chunk.has_value());

    const auto ops = extract_opcodes(*chunk);
    const std::vector<ely::core::Opcode> expected = {
        ely::core::Opcode::Constant,
        ely::core::Opcode::DefineGlobal,
        ely::core::Opcode::Constant,
        ely::core::Opcode::DefineGlobal,
        ely::core::Opcode::GetGlobal,
        ely::core::Opcode::GetGlobal,
        ely::core::Opcode::Add,
        ely::core::Opcode::Print,
        ely::core::Opcode::Nop,
        ely::core::Opcode::Return
    };

    REQUIRE(ops == expected);
}

TEST_CASE("Compiler local scope and variables", "[compiler]") {
    std::string_view src =
        "var a = 1;\n"
        "{\n"
        "    var b = 2;\n"
        "    print a + b;\n"
        "}\n";

    ely::compiler::Compiler comp(src);
    auto chunk = comp.compile();
    REQUIRE(chunk.has_value());

    const auto ops = extract_opcodes(*chunk);
    const std::vector<ely::core::Opcode> expected = {
        ely::core::Opcode::Constant,
        ely::core::Opcode::DefineGlobal,
        ely::core::Opcode::Constant,
        ely::core::Opcode::GetGlobal,
        ely::core::Opcode::GetLocal,
        ely::core::Opcode::Add,
        ely::core::Opcode::Print,
        ely::core::Opcode::Pop, // popped local 'b' at scope exit
        ely::core::Opcode::Nop,
        ely::core::Opcode::Return
    };

    REQUIRE(ops == expected);
}

TEST_CASE("Compiler if statement", "[compiler]") {
    std::string_view src =
        "if (true) {\n"
        "    print 1;\n"
        "} else {\n"
        "    print 2;\n"
        "}\n";

    ely::compiler::Compiler comp(src);
    auto chunk = comp.compile();
    REQUIRE(chunk.has_value());

    const auto ops = extract_opcodes(*chunk);
    const std::vector<ely::core::Opcode> expected = {
        ely::core::Opcode::True,
        ely::core::Opcode::JumpZero,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Constant,
        ely::core::Opcode::Print,
        ely::core::Opcode::Jump,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Constant,
        ely::core::Opcode::Print,
        ely::core::Opcode::Nop,
        ely::core::Opcode::Return
    };

    REQUIRE(ops == expected);
}

TEST_CASE("Compiler while loop statement", "[compiler]") {
    std::string_view src =
        "var i = 0;\n"
        "while (i < 3) {\n"
        "    print i;\n"
        "    i = i + 1;\n"
        "}\n";

    ely::compiler::Compiler comp(src);
    auto chunk = comp.compile();
    REQUIRE(chunk.has_value());

    const auto ops = extract_opcodes(*chunk);
    const std::vector<ely::core::Opcode> expected = {
        ely::core::Opcode::Constant,
        ely::core::Opcode::DefineGlobal,
        ely::core::Opcode::GetGlobal,
        ely::core::Opcode::Constant,
        ely::core::Opcode::Less,
        ely::core::Opcode::JumpZero,
        ely::core::Opcode::Pop,
        ely::core::Opcode::GetGlobal,
        ely::core::Opcode::Print,
        ely::core::Opcode::GetGlobal,
        ely::core::Opcode::Constant,
        ely::core::Opcode::Add,
        ely::core::Opcode::SetGlobal,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Loop,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Nop,
        ely::core::Opcode::Return
    };

    REQUIRE(ops == expected);
}

TEST_CASE("Compiler for loop statement", "[compiler]") {
    std::string_view src =
        "for (var i = 0; i < 3; i = i + 1) {\n"
        "    print i;\n"
        "}\n";

    ely::compiler::Compiler comp(src);
    auto chunk = comp.compile();
    REQUIRE(chunk.has_value());

    const auto ops = extract_opcodes(*chunk);
    const std::vector<ely::core::Opcode> expected = {
        ely::core::Opcode::Constant,
        ely::core::Opcode::GetLocal,
        ely::core::Opcode::Constant,
        ely::core::Opcode::Less,
        ely::core::Opcode::JumpZero,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Jump,
        ely::core::Opcode::GetLocal,
        ely::core::Opcode::Constant,
        ely::core::Opcode::Add,
        ely::core::Opcode::SetLocal,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Loop,
        ely::core::Opcode::GetLocal,
        ely::core::Opcode::Print,
        ely::core::Opcode::Loop,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Nop,
        ely::core::Opcode::Return
    };

    REQUIRE(ops == expected);
}

TEST_CASE("Compiler logical and/or", "[compiler]") {
    std::string_view src = "false && true || 42;";

    ely::compiler::Compiler comp(src);
    auto chunk = comp.compile();
    REQUIRE(chunk.has_value());

    const auto ops = extract_opcodes(*chunk);
    const std::vector<ely::core::Opcode> expected = {
        ely::core::Opcode::False,
        ely::core::Opcode::JumpZero,
        ely::core::Opcode::Pop,
        ely::core::Opcode::True,
        ely::core::Opcode::JumpZero,
        ely::core::Opcode::Jump,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Constant,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Nop,
        ely::core::Opcode::Return
    };

    REQUIRE(ops == expected);
}

TEST_CASE("Compiler function call expression", "[compiler]") {
    std::string_view src = "foo(1, 2);";

    ely::compiler::Compiler comp(src);
    auto chunk = comp.compile();
    REQUIRE(chunk.has_value());

    const auto ops = extract_opcodes(*chunk);
    const std::vector<ely::core::Opcode> expected = {
        ely::core::Opcode::GetGlobal,
        ely::core::Opcode::Constant,
        ely::core::Opcode::Constant,
        ely::core::Opcode::Call,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Nop,
        ely::core::Opcode::Return
    };

    REQUIRE(ops == expected);
}

TEST_CASE("Compiler function declaration", "[compiler]") {
    std::string_view src =
        "func add(a, b) {\n"
        "    print a + b;\n"
        "}\n";

    ely::compiler::Compiler comp(src);
    auto chunk = comp.compile();
    REQUIRE(chunk.has_value());

    const auto ops = extract_opcodes(*chunk);
    const std::vector<ely::core::Opcode> expected = {
        ely::core::Opcode::Closure,
        ely::core::Opcode::DefineGlobal,
        ely::core::Opcode::Nop,
        ely::core::Opcode::Return
    };

    REQUIRE(ops == expected);
    REQUIRE(chunk->function_constants().size() == 1);

    const auto& func_desc = chunk->function_constants()[0];
    REQUIRE(func_desc.name == "add");
    REQUIRE(func_desc.arity == 2);
    REQUIRE(func_desc.chunk != nullptr);

    const auto func_ops = extract_opcodes(*func_desc.chunk);
    const std::vector<ely::core::Opcode> expected_func_ops = {
        ely::core::Opcode::GetLocal,
        ely::core::Opcode::GetLocal,
        ely::core::Opcode::Add,
        ely::core::Opcode::Print,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Pop,
        ely::core::Opcode::Nop,
        ely::core::Opcode::Return
    };

    REQUIRE(func_ops == expected_func_ops);
}
