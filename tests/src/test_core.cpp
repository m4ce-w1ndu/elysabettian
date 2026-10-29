#include <catch2/catch_test_macros.hpp>
#include <ely/core/value.hpp>
#include <ely/core/chunk.hpp>
#include <ely/core/opcode.hpp>

TEST_CASE("Value NaN-boxing operations", "[core][value]") {
    SECTION("Numbers round-trip correctly") {
        const auto val = ely::core::Value::number(42.5);
        REQUIRE(val.is_number());
        REQUIRE_FALSE(val.is_null());
        REQUIRE_FALSE(val.is_boolean());
        REQUIRE_FALSE(val.is_obj());
        REQUIRE(val.as_number() == 42.5);
    }

    SECTION("Booleans round-trip correctly") {
        const auto val_true = ely::core::Value::boolean(true);
        const auto val_false = ely::core::Value::boolean(false);

        REQUIRE(val_true.is_boolean());
        REQUIRE(val_true.as_boolean() == true);
        REQUIRE(val_false.is_boolean());
        REQUIRE(val_false.as_boolean() == false);
    }

    SECTION("Null value representation") {
        const auto val_null = ely::core::Value::null();
        REQUIRE(val_null.is_null());
        REQUIRE_FALSE(val_null.is_boolean());
        REQUIRE_FALSE(val_null.is_number());
        REQUIRE_FALSE(val_null.is_obj());
    }

    SECTION("Object NaN-boxed pointer/index") {
        const auto val_obj = ely::core::Value::obj(12345);
        REQUIRE(val_obj.is_obj());
        REQUIRE(val_obj.as_obj() == 12345);
    }
}

TEST_CASE("Chunk bytecode and line compression", "[core][chunk]") {
    ely::core::Chunk chunk;

    chunk.write_opcode(ely::core::Opcode::Nop, 1);
    chunk.write_opcode(ely::core::Opcode::Return, 1);
    chunk.write_opcode(ely::core::Opcode::Pop, 2);

    REQUIRE(chunk.size() == 3);
    REQUIRE(chunk.get_line(0) == 1);
    REQUIRE(chunk.get_line(1) == 1);
    REQUIRE(chunk.get_line(2) == 2);
}
