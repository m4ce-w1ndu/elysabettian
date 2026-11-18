#include "test_harness.h"

#include <vtx/stdlib/native.h>
#include <vtx/core/value.h>

#include <stdio.h>
#include <string.h>

static VtxValue dummy_native(uint8_t argc, const VtxValue* args) {
    (void)argc;
    (void)args;
    return vtx_value_number(42.0);
}

static void test_hash_string(void) {
    uint32_t hash1 = vtx_hash_string("foo");
    uint32_t hash2 = vtx_hash_string("foo");
    uint32_t hash3 = vtx_hash_string("bar");

    TEST_ASSERT_EQ(hash1, hash2);
    TEST_ASSERT(hash1 != hash3);
    TEST_ASSERT(hash1 != 0);

    // Empty string should produce offset basis
    TEST_ASSERT_EQ(vtx_hash_string(""), 0x811C9DC5U);
}

static void test_table_init_free_clear(void) {
    VtxTable table;
    vtx_table_init(&table);

    TEST_ASSERT_EQ(table.count, 0);
    TEST_ASSERT_EQ(table.capacity, 0);
    TEST_ASSERT(table.entries == NULL);

    // Insert an item so capacity grows
    vtx_table_set_value(&table, "item", vtx_value_number(123.0));
    TEST_ASSERT_EQ(table.count, 1);
    TEST_ASSERT(table.capacity >= 8);
    TEST_ASSERT(table.entries != NULL);

    // Test clear preserves capacity but zeroes count and empties entries
    size_t old_cap = table.capacity;
    vtx_table_clear(&table);
    TEST_ASSERT_EQ(table.count, 0);
    TEST_ASSERT_EQ(table.capacity, old_cap);
    TEST_ASSERT(vtx_table_find(&table, "item") == NULL);

    // Test free resets everything to null/zero
    vtx_table_free(&table);
    TEST_ASSERT_EQ(table.count, 0);
    TEST_ASSERT_EQ(table.capacity, 0);
    TEST_ASSERT(table.entries == NULL);
}

static void test_table_set_and_find_value(void) {
    VtxTable table;
    vtx_table_init(&table);

    // Insert new key
    bool is_new = vtx_table_set_value(&table, "alpha", vtx_value_number(1.0));
    TEST_ASSERT(is_new);
    TEST_ASSERT_EQ(table.count, 1);

    // Find key
    VtxEntry* entry = vtx_table_find(&table, "alpha");
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT_EQ(entry->status, VTX_ENTRY_OCCUPIED);
    TEST_ASSERT(vtx_value_equal(entry->value, vtx_value_number(1.0)));

    // Non-existent key returns NULL
    TEST_ASSERT(vtx_table_find(&table, "beta") == NULL);

    // Overwrite existing key
    is_new = vtx_table_set_value(&table, "alpha", vtx_value_number(2.0));
    TEST_ASSERT(!is_new);
    TEST_ASSERT_EQ(table.count, 1);

    entry = vtx_table_find(&table, "alpha");
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(vtx_value_equal(entry->value, vtx_value_number(2.0)));

    vtx_table_free(&table);
}

static void test_table_set_and_find_native_and_id(void) {
    VtxTable table;
    vtx_table_init(&table);

    // Set native function pointer
    bool is_new = vtx_table_set_native(&table, "print", dummy_native);
    TEST_ASSERT(is_new);
    VtxEntry* entry = vtx_table_find(&table, "print");
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT_EQ(entry->native, dummy_native);

    // Set size_t identifier
    is_new = vtx_table_set_id(&table, "my_method", 10042);
    TEST_ASSERT(is_new);
    entry = vtx_table_find(&table, "my_method");
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT_EQ(entry->id, (size_t)10042);

    TEST_ASSERT_EQ(table.count, 2);

    vtx_table_free(&table);
}

static void test_table_delete(void) {
    VtxTable table;
    vtx_table_init(&table);

    vtx_table_set_value(&table, "k1", vtx_value_bool(true));
    vtx_table_set_value(&table, "k2", vtx_value_bool(false));
    TEST_ASSERT_EQ(table.count, 2);

    // Delete existing key
    bool deleted = vtx_table_delete(&table, "k1");
    TEST_ASSERT(deleted);
    TEST_ASSERT_EQ(table.count, 1);
    TEST_ASSERT(vtx_table_find(&table, "k1") == NULL);

    // Delete non-existent key returns false
    deleted = vtx_table_delete(&table, "k1");
    TEST_ASSERT(!deleted);

    deleted = vtx_table_delete(&table, "nonexistent");
    TEST_ASSERT(!deleted);

    // Other key still intact
    VtxEntry* e2 = vtx_table_find(&table, "k2");
    TEST_ASSERT(e2 != NULL);
    TEST_ASSERT(vtx_value_as_bool(e2->value) == false);

    vtx_table_free(&table);
}

static void test_table_tombstone_reuse(void) {
    VtxTable table;
    vtx_table_init(&table);

    // Insert, delete, then insert again to exercise tombstone reuse
    vtx_table_set_value(&table, "reused_key", vtx_value_number(10.0));
    TEST_ASSERT_EQ(table.count, 1);

    bool deleted = vtx_table_delete(&table, "reused_key");
    TEST_ASSERT(deleted);
    TEST_ASSERT_EQ(table.count, 0);
    TEST_ASSERT(vtx_table_find(&table, "reused_key") == NULL);

    // Re-insert same key into the tombstone slot
    bool is_new = vtx_table_set_value(&table, "reused_key", vtx_value_number(20.0));
    TEST_ASSERT(is_new);
    TEST_ASSERT_EQ(table.count, 1);

    VtxEntry* entry = vtx_table_find(&table, "reused_key");
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT_FLOAT_EQ(vtx_value_as_number(entry->value), 20.0, 1e-9);

    vtx_table_free(&table);
}

static void test_table_resize_many_keys(void) {
    VtxTable table;
    vtx_table_init(&table);

    // Insert 100 entries to trigger multiple reallocations
    char key_buf[32];
    for (int i = 0; i < 100; ++i) {
        snprintf(key_buf, sizeof(key_buf), "key_%d", i);
        vtx_table_set_value(&table, key_buf, vtx_value_number((double)i));
    }

    TEST_ASSERT_EQ(table.count, 100);
    TEST_ASSERT(table.capacity >= 128);

    // Verify all 100 keys are preserved and correct after resizes
    for (int i = 0; i < 100; ++i) {
        snprintf(key_buf, sizeof(key_buf), "key_%d", i);
        VtxEntry* entry = vtx_table_find(&table, key_buf);
        TEST_ASSERT(entry != NULL);
        TEST_ASSERT_FLOAT_EQ(vtx_value_as_number(entry->value), (double)i, 1e-9);
    }

    vtx_table_free(&table);
}

static void test_table_add_all(void) {
    VtxTable src;
    VtxTable dest;
    vtx_table_init(&src);
    vtx_table_init(&dest);

    vtx_table_set_value(&src, "s1", vtx_value_number(11.0));
    vtx_table_set_value(&src, "s2", vtx_value_number(22.0));

    vtx_table_set_value(&dest, "d1", vtx_value_number(99.0));

    vtx_table_add_all(&dest, &src);

    TEST_ASSERT_EQ(dest.count, 3);
    TEST_ASSERT(vtx_table_find(&dest, "s1") != NULL);
    TEST_ASSERT(vtx_table_find(&dest, "s2") != NULL);
    TEST_ASSERT(vtx_table_find(&dest, "d1") != NULL);

    vtx_table_free(&src);
    vtx_table_free(&dest);
}

void run_table_tests(void) {
    RUN_TEST(test_hash_string);
    RUN_TEST(test_table_init_free_clear);
    RUN_TEST(test_table_set_and_find_value);
    RUN_TEST(test_table_set_and_find_native_and_id);
    RUN_TEST(test_table_delete);
    RUN_TEST(test_table_tombstone_reuse);
    RUN_TEST(test_table_resize_many_keys);
    RUN_TEST(test_table_add_all);
}
