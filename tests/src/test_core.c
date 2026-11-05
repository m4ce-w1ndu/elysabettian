#include "test_harness.h"
#include <ely/core/value.h>
#include <ely/core/chunk.h>
#include <ely/core/opcode.h>
#include <ely/core/debug.h>

static void test_nan_boxing_numbers(void) {
    ElyValue val = ely_value_number(42.5);
    TEST_ASSERT(ely_value_is_number(val));
    TEST_ASSERT(!ely_value_is_null(val));
    TEST_ASSERT(!ely_value_is_bool(val));
    TEST_ASSERT(!ely_value_is_obj(val));
    TEST_ASSERT_FLOAT_EQ(ely_value_as_number(val), 42.5, 1e-9);

    // Negative numbers and zero
    val = ely_value_number(-100.25);
    TEST_ASSERT(ely_value_is_number(val));
    TEST_ASSERT_FLOAT_EQ(ely_value_as_number(val), -100.25, 1e-9);

    val = ely_value_number(0.0);
    TEST_ASSERT(ely_value_is_number(val));
    TEST_ASSERT_FLOAT_EQ(ely_value_as_number(val), 0.0, 1e-9);
}

static void test_nan_boxing_booleans(void) {
    ElyValue val_true = ely_value_bool(true);
    ElyValue val_false = ely_value_bool(false);

    TEST_ASSERT(ely_value_is_bool(val_true));
    TEST_ASSERT_EQ(ely_value_as_bool(val_true), true);
    TEST_ASSERT(!ely_value_is_number(val_true));
    TEST_ASSERT(!ely_value_is_null(val_true));
    TEST_ASSERT(!ely_value_is_obj(val_true));

    TEST_ASSERT(ely_value_is_bool(val_false));
    TEST_ASSERT_EQ(ely_value_as_bool(val_false), false);
    TEST_ASSERT(!ely_value_is_number(val_false));
    TEST_ASSERT(!ely_value_is_null(val_false));
    TEST_ASSERT(!ely_value_is_obj(val_false));
}

static void test_nan_boxing_null(void) {
    ElyValue val_null = ely_value_null();
    TEST_ASSERT(ely_value_is_null(val_null));
    TEST_ASSERT(!ely_value_is_bool(val_null));
    TEST_ASSERT(!ely_value_is_number(val_null));
    TEST_ASSERT(!ely_value_is_obj(val_null));
}

static void test_nan_boxing_truthy(void) {
    // Null and false are falsy
    TEST_ASSERT(!ely_value_is_truthy(ely_value_null()));
    TEST_ASSERT(!ely_value_is_truthy(ely_value_bool(false)));

    // Everything else is truthy
    TEST_ASSERT(ely_value_is_truthy(ely_value_bool(true)));
    TEST_ASSERT(ely_value_is_truthy(ely_value_number(0.0)));
    TEST_ASSERT(ely_value_is_truthy(ely_value_number(42.0)));
    TEST_ASSERT(ely_value_is_truthy(ely_value_obj(0x12345678ULL)));
}

static void test_nan_boxing_object_pointers(void) {
    uint64_t payload = 0x00007FFEEDDCCBBAULL;
    ElyValue val_obj = ely_value_obj(payload);

    TEST_ASSERT(ely_value_is_obj(val_obj));
    TEST_ASSERT(!ely_value_is_number(val_obj));
    TEST_ASSERT(!ely_value_is_null(val_obj));
    TEST_ASSERT(!ely_value_is_bool(val_obj));
    TEST_ASSERT_EQ(ely_value_as_obj(val_obj), payload);
}

static void test_nan_boxing_equality(void) {
    ElyValue a = ely_value_number(12.34);
    ElyValue b = ely_value_number(12.34);
    ElyValue c = ely_value_number(56.78);

    TEST_ASSERT(ely_value_equal(a, b));
    TEST_ASSERT(!ely_value_equal(a, c));

    ElyValue n1 = ely_value_null();
    ElyValue n2 = ely_value_null();
    TEST_ASSERT(ely_value_equal(n1, n2));

    ElyValue t = ely_value_bool(true);
    ElyValue f = ely_value_bool(false);
    TEST_ASSERT(!ely_value_equal(t, f));
    TEST_ASSERT(ely_value_equal(t, ely_value_bool(true)));
}

static void test_chunk_operations(void) {
    ElyChunk chunk;
    ely_chunk_init(&chunk);

    // Initial state
    TEST_ASSERT_EQ(chunk.code_count, 0);
    TEST_ASSERT_EQ(chunk.constants_count, 0);
    TEST_ASSERT_EQ(chunk.lines_count, 0);

    // Add constants
    size_t c1 = ely_chunk_add_constant(&chunk, ely_value_number(1.23));
    size_t c2 = ely_chunk_add_constant(&chunk, ely_value_number(4.56));
    TEST_ASSERT_EQ(c1, 0);
    TEST_ASSERT_EQ(c2, 1);
    TEST_ASSERT_EQ(chunk.constants_count, 2);
    TEST_ASSERT_FLOAT_EQ(ely_value_as_number(chunk.constants[0]), 1.23, 1e-9);
    TEST_ASSERT_FLOAT_EQ(ely_value_as_number(chunk.constants[1]), 4.56, 1e-9);

    // Write instructions across different lines
    ely_chunk_write(&chunk, (uint8_t)ELY_OP_CONSTANT, 1);
    ely_chunk_write(&chunk, (uint8_t)c1, 1);
    ely_chunk_write(&chunk, (uint8_t)ELY_OP_CONSTANT, 1);
    ely_chunk_write(&chunk, (uint8_t)c2, 1);
    ely_chunk_write(&chunk, (uint8_t)ELY_OP_ADD, 2);
    ely_chunk_write(&chunk, (uint8_t)ELY_OP_RETURN, 3);

    TEST_ASSERT_EQ(chunk.code_count, 6);

    // RLE line checkpoints check: line 1, 2, 3 should produce only 3 checkpoints
    TEST_ASSERT_EQ(chunk.lines_count, 3);

    // Binary search line lookup
    TEST_ASSERT_EQ(ely_chunk_get_line(&chunk, 0), 1);
    TEST_ASSERT_EQ(ely_chunk_get_line(&chunk, 1), 1);
    TEST_ASSERT_EQ(ely_chunk_get_line(&chunk, 2), 1);
    TEST_ASSERT_EQ(ely_chunk_get_line(&chunk, 3), 1);
    TEST_ASSERT_EQ(ely_chunk_get_line(&chunk, 4), 2);
    TEST_ASSERT_EQ(ely_chunk_get_line(&chunk, 5), 3);

    ely_chunk_free(&chunk);
    TEST_ASSERT_EQ(chunk.code_count, 0);
    TEST_ASSERT_EQ(chunk.code, NULL);
}

static void test_disassembler(void) {
    ElyChunk chunk;
    ely_chunk_init(&chunk);

    size_t const_idx = ely_chunk_add_constant(&chunk, ely_value_number(99.0));
    ely_chunk_write(&chunk, (uint8_t)ELY_OP_CONSTANT, 1);
    ely_chunk_write(&chunk, (uint8_t)const_idx, 1);
    ely_chunk_write(&chunk, (uint8_t)ELY_OP_RETURN, 2);

    // Test instruction disassembly steps
    size_t next_offset = ely_debug_disassemble_instruction(&chunk, 0);
    TEST_ASSERT_EQ(next_offset, 2);

    next_offset = ely_debug_disassemble_instruction(&chunk, 2);
    TEST_ASSERT_EQ(next_offset, 3);

    ely_chunk_free(&chunk);
}

void run_core_tests(void) {
    RUN_TEST(test_nan_boxing_numbers);
    RUN_TEST(test_nan_boxing_booleans);
    RUN_TEST(test_nan_boxing_null);
    RUN_TEST(test_nan_boxing_truthy);
    RUN_TEST(test_nan_boxing_object_pointers);
    RUN_TEST(test_nan_boxing_equality);
    RUN_TEST(test_chunk_operations);
    RUN_TEST(test_disassembler);
}
