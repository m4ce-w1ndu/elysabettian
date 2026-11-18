#include "test_harness.h"
#include <vtx/core/value.h>
#include <vtx/core/chunk.h>
#include <vtx/core/opcode.h>
#include <vtx/core/debug.h>

static void test_nan_boxing_numbers(void) {
    VtxValue val = vtx_value_number(42.5);
    TEST_ASSERT(vtx_value_is_number(val));
    TEST_ASSERT(!vtx_value_is_null(val));
    TEST_ASSERT(!vtx_value_is_bool(val));
    TEST_ASSERT(!vtx_value_is_obj(val));
    TEST_ASSERT_FLOAT_EQ(vtx_value_as_number(val), 42.5, 1e-9);

    // Negative numbers and zero
    val = vtx_value_number(-100.25);
    TEST_ASSERT(vtx_value_is_number(val));
    TEST_ASSERT_FLOAT_EQ(vtx_value_as_number(val), -100.25, 1e-9);

    val = vtx_value_number(0.0);
    TEST_ASSERT(vtx_value_is_number(val));
    TEST_ASSERT_FLOAT_EQ(vtx_value_as_number(val), 0.0, 1e-9);
}

static void test_nan_boxing_booleans(void) {
    VtxValue val_true = vtx_value_bool(true);
    VtxValue val_false = vtx_value_bool(false);

    TEST_ASSERT(vtx_value_is_bool(val_true));
    TEST_ASSERT_EQ(vtx_value_as_bool(val_true), true);
    TEST_ASSERT(!vtx_value_is_number(val_true));
    TEST_ASSERT(!vtx_value_is_null(val_true));
    TEST_ASSERT(!vtx_value_is_obj(val_true));

    TEST_ASSERT(vtx_value_is_bool(val_false));
    TEST_ASSERT_EQ(vtx_value_as_bool(val_false), false);
    TEST_ASSERT(!vtx_value_is_number(val_false));
    TEST_ASSERT(!vtx_value_is_null(val_false));
    TEST_ASSERT(!vtx_value_is_obj(val_false));
}

static void test_nan_boxing_null(void) {
    VtxValue val_null = vtx_value_null();
    TEST_ASSERT(vtx_value_is_null(val_null));
    TEST_ASSERT(!vtx_value_is_bool(val_null));
    TEST_ASSERT(!vtx_value_is_number(val_null));
    TEST_ASSERT(!vtx_value_is_obj(val_null));
}

static void test_nan_boxing_truthy(void) {
    // Null and false are falsy
    TEST_ASSERT(!vtx_value_is_truthy(vtx_value_null()));
    TEST_ASSERT(!vtx_value_is_truthy(vtx_value_bool(false)));

    // Everything else is truthy
    TEST_ASSERT(vtx_value_is_truthy(vtx_value_bool(true)));
    TEST_ASSERT(vtx_value_is_truthy(vtx_value_number(0.0)));
    TEST_ASSERT(vtx_value_is_truthy(vtx_value_number(42.0)));
    TEST_ASSERT(vtx_value_is_truthy(vtx_value_obj(0x12345678ULL)));
}

static void test_nan_boxing_object_pointers(void) {
    uint64_t payload = 0x00007FFEEDDCCBBAULL;
    VtxValue val_obj = vtx_value_obj(payload);

    TEST_ASSERT(vtx_value_is_obj(val_obj));
    TEST_ASSERT(!vtx_value_is_number(val_obj));
    TEST_ASSERT(!vtx_value_is_null(val_obj));
    TEST_ASSERT(!vtx_value_is_bool(val_obj));
    TEST_ASSERT_EQ(vtx_value_as_obj(val_obj), payload);
}

static void test_nan_boxing_equality(void) {
    VtxValue a = vtx_value_number(12.34);
    VtxValue b = vtx_value_number(12.34);
    VtxValue c = vtx_value_number(56.78);

    TEST_ASSERT(vtx_value_equal(a, b));
    TEST_ASSERT(!vtx_value_equal(a, c));

    VtxValue n1 = vtx_value_null();
    VtxValue n2 = vtx_value_null();
    TEST_ASSERT(vtx_value_equal(n1, n2));

    VtxValue t = vtx_value_bool(true);
    VtxValue f = vtx_value_bool(false);
    TEST_ASSERT(!vtx_value_equal(t, f));
    TEST_ASSERT(vtx_value_equal(t, vtx_value_bool(true)));
}

static void test_chunk_operations(void) {
    VtxChunk chunk;
    vtx_chunk_init(&chunk);

    // Initial state
    TEST_ASSERT_EQ(chunk.code_count, 0);
    TEST_ASSERT_EQ(chunk.constants_count, 0);
    TEST_ASSERT_EQ(chunk.lines_count, 0);

    // Add constants
    size_t c1 = vtx_chunk_add_constant(&chunk, vtx_value_number(1.23));
    size_t c2 = vtx_chunk_add_constant(&chunk, vtx_value_number(4.56));
    TEST_ASSERT_EQ(c1, 0);
    TEST_ASSERT_EQ(c2, 1);
    TEST_ASSERT_EQ(chunk.constants_count, 2);
    TEST_ASSERT_FLOAT_EQ(vtx_value_as_number(chunk.constants[0]), 1.23, 1e-9);
    TEST_ASSERT_FLOAT_EQ(vtx_value_as_number(chunk.constants[1]), 4.56, 1e-9);

    // Write instructions across different lines
    vtx_chunk_write(&chunk, (uint8_t)VTX_OP_CONSTANT, 1);
    vtx_chunk_write(&chunk, (uint8_t)c1, 1);
    vtx_chunk_write(&chunk, (uint8_t)VTX_OP_CONSTANT, 1);
    vtx_chunk_write(&chunk, (uint8_t)c2, 1);
    vtx_chunk_write(&chunk, (uint8_t)VTX_OP_ADD, 2);
    vtx_chunk_write(&chunk, (uint8_t)VTX_OP_RETURN, 3);

    TEST_ASSERT_EQ(chunk.code_count, 6);

    // RLE line checkpoints check: line 1, 2, 3 should produce only 3 checkpoints
    TEST_ASSERT_EQ(chunk.lines_count, 3);

    // Binary search line lookup
    TEST_ASSERT_EQ(vtx_chunk_get_line(&chunk, 0), 1);
    TEST_ASSERT_EQ(vtx_chunk_get_line(&chunk, 1), 1);
    TEST_ASSERT_EQ(vtx_chunk_get_line(&chunk, 2), 1);
    TEST_ASSERT_EQ(vtx_chunk_get_line(&chunk, 3), 1);
    TEST_ASSERT_EQ(vtx_chunk_get_line(&chunk, 4), 2);
    TEST_ASSERT_EQ(vtx_chunk_get_line(&chunk, 5), 3);

    vtx_chunk_free(&chunk);
    TEST_ASSERT_EQ(chunk.code_count, 0);
    TEST_ASSERT_EQ(chunk.code, NULL);
}

static void test_disassembler(void) {
    VtxChunk chunk;
    vtx_chunk_init(&chunk);

    size_t const_idx = vtx_chunk_add_constant(&chunk, vtx_value_number(99.0));
    vtx_chunk_write(&chunk, (uint8_t)VTX_OP_CONSTANT, 1);
    vtx_chunk_write(&chunk, (uint8_t)const_idx, 1);
    vtx_chunk_write(&chunk, (uint8_t)VTX_OP_RETURN, 2);

    // Test instruction disassembly steps
    size_t next_offset = vtx_debug_disassemble_instruction(&chunk, 0);
    TEST_ASSERT_EQ(next_offset, 2);

    next_offset = vtx_debug_disassemble_instruction(&chunk, 2);
    TEST_ASSERT_EQ(next_offset, 3);

    vtx_chunk_free(&chunk);
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
