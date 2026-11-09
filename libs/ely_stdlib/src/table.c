#include <ely/stdlib/native.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

uint32_t ely_hash_string(const char* str) {
    const uint32_t basis = 0x811C9DC5U;
    const uint32_t prime = 0x01000193U;

    uint32_t hash = basis;

    for (const char* c = str; *c != '\0'; c++) {
        hash ^= (uint8_t)*c;
        hash *= prime;
    }

    return hash;
}

static inline ElyEntry* find_entry(ElyEntry* entries, size_t capacity, uint32_t key) {
    uint32_t index = key & (uint32_t)(capacity - 1);

    ElyEntry* tombstone = NULL;
    ElyEntry* empty = NULL;
    ElyEntry* found = NULL;

    while (true) {
        if (tombstone == NULL && entries[index].status == ELY_ENTRY_TOMBSTONE) {
            tombstone = &entries[index];
        }

        if (entries[index].key == key && entries[index].status == ELY_ENTRY_OCCUPIED) {
            found = &entries[index];
            break;
        }

        if (empty == NULL && entries[index].status == ELY_ENTRY_EMPTY) {
            empty = &entries[index];
            break;
        }

        index = (index + 1) & (uint32_t)(capacity - 1);
    }

    return found ? found : tombstone ? tombstone : empty;
}

static inline void adjust_capacity(ElyTable* table, size_t new_capacity) {
    ElyEntry* new_entries = (ElyEntry*)malloc(sizeof(struct ElyEntry) * new_capacity);
    assert(new_entries != NULL && "Out of memory, cannot reallocate hash table.");

    // Reinitialise the table
    for (size_t i = 0; i < new_capacity; ++i) {
        new_entries[i].status = ELY_ENTRY_EMPTY;
        new_entries[i].value = 0;
        new_entries[i].key = 0;
    }

    // Re-hash existing elements
    table->count = 0;
    for (size_t i = 0; i < table->capacity; ++i) {
        const ElyEntry* entry = &table->entries[i];
        if (entry->status != ELY_ENTRY_OCCUPIED) continue;
        ElyEntry* dest = find_entry(new_entries, new_capacity, entry->key);
        *dest = *entry;
        table->count++;
    }

    free(table->entries);
    table->entries = new_entries;
    table->capacity = new_capacity;
}

void ely_table_init(ElyTable* table) {
    assert(table != NULL && "Table pointer is NULL.");
    table->entries = NULL;
    table->capacity = 0;
    table->count = 0;
}

void ely_table_free(ElyTable* table) {
    assert(table != NULL && "Table pointer is NULL.");
    free(table->entries);
    ely_table_init(table);
}

void ely_table_clear(ElyTable* table) {
    assert(table != NULL && "Table pointer is NULL.");
    for (size_t i = 0; i < table->capacity; ++i) {
        table->entries[i].status = ELY_ENTRY_EMPTY;
    }
    table->count = 0;
}

ElyEntry* ely_table_find(const ElyTable* table, const char* key) {
    assert(table != NULL && "Table pointer is NULL.");
    assert(key != NULL && "Key pointer is NULL.");
    const uint32_t hash = ely_hash_string(key);
    return ely_table_find_by_hash(table, hash);
}

ElyEntry* ely_table_find_by_hash(const ElyTable* table, uint32_t hash) {
    assert(table != NULL && "Table pointer is NULL.");
    if (table->count == 0) return NULL;
    ElyEntry* entry = find_entry(table->entries, table->capacity, hash);
    return entry->status == ELY_ENTRY_OCCUPIED ? entry : NULL;
}

bool ely_table_set_value(ElyTable* table, const char* key, ElyValue value) {
    assert(table != NULL && "Table pointer is NULL.");
    assert(key != NULL && "Key pointer is NULL.");
    const uint32_t hash = ely_hash_string(key);
    return ely_table_set_value_by_hash(table, hash, value);
}

bool ely_table_set_value_by_hash(ElyTable* table, uint32_t hash, ElyValue value) {
    assert(table != NULL && "Table pointer is NULL.");

    if (table->count + 1 > (table->capacity * 3) / 4) {
        size_t new_capacity = table->capacity < 8 ? 8 : table->capacity * 2;
        adjust_capacity(table, new_capacity);
    }

    bool is_new_entry = false;
    ElyEntry* slot = find_entry(table->entries, table->capacity, hash);

    if (slot->status != ELY_ENTRY_OCCUPIED) {
        table->count++;
        is_new_entry = true;
    }

    slot->key = hash;
    slot->status = ELY_ENTRY_OCCUPIED;
    slot->value = value;

    return is_new_entry;
}

bool ely_table_set_native(ElyTable* table, const char* key, ElyNativeFn native) {
    assert(table != NULL && "Table pointer is NULL.");
    assert(key != NULL && "Key pointer is NULL.");
    const uint32_t hash = ely_hash_string(key);
    return ely_table_set_native_by_hash(table, hash, native);
}

bool ely_table_set_native_by_hash(ElyTable* table, uint32_t hash, ElyNativeFn native) {
    assert(table != NULL && "Table pointer is NULL.");

    if (table->count + 1 > (table->capacity * 3) / 4) {
        size_t new_capacity = table->capacity < 8 ? 8 : table->capacity * 2;
        adjust_capacity(table, new_capacity);
    }

    bool is_new_entry = false;
    ElyEntry* slot = find_entry(table->entries, table->capacity, hash);

    if (slot->status != ELY_ENTRY_OCCUPIED) {
        table->count++;
        is_new_entry = true;
    }

    slot->key = hash;
    slot->status = ELY_ENTRY_OCCUPIED;
    slot->native = native;

    return is_new_entry;
}

bool ely_table_set_id(ElyTable* table, const char* key, size_t id) {
    assert(table != NULL && "Table pointer is NULL.");
    assert(key != NULL && "Key pointer is NULL.");
    const uint32_t hash = ely_hash_string(key);
    return ely_table_set_id_by_hash(table, hash, id);
}

bool ely_table_set_id_by_hash(ElyTable* table, uint32_t hash, size_t id) {
    assert(table != NULL && "Table pointer is NULL.");

    if (table->count + 1 > (table->capacity * 3) / 4) {
        size_t new_capacity = table->capacity < 8 ? 8 : table->capacity * 2;
        adjust_capacity(table, new_capacity);
    }

    bool is_new_entry = false;
    ElyEntry* slot = find_entry(table->entries, table->capacity, hash);

    if (slot->status != ELY_ENTRY_OCCUPIED) {
        table->count++;
        is_new_entry = true;
    }

    slot->key = hash;
    slot->status = ELY_ENTRY_OCCUPIED;
    slot->id = id;

    return is_new_entry;
}

bool ely_table_delete(ElyTable* table, const char* key) {
    assert(table != NULL && "Table pointer is NULL.");
    assert(key != NULL && "Key pointer is NULL.");
    
    const uint32_t hash = ely_hash_string(key);
    return ely_table_delete_by_hash(table, hash);
}

bool ely_table_delete_by_hash(ElyTable* table, uint32_t hash) {
    assert(table != NULL && "Table pointer is NULL.");

    if (table->count == 0) return false;

    ElyEntry* to_delete = find_entry(table->entries, table->capacity, hash);
    if (to_delete->status != ELY_ENTRY_OCCUPIED) return false;

    to_delete->status = ELY_ENTRY_TOMBSTONE;
    table->count--;
    return true;
}

void ely_table_add_all(ElyTable* dest, const ElyTable* src) {
    assert(dest != NULL && "Destination table pointer is NULL.");
    assert(src != NULL && "Source table pointer is NULL.");

    for (size_t i = 0; i < src->capacity; ++i) {
        const ElyEntry* entry = &src->entries[i];
        if (entry->status != ELY_ENTRY_OCCUPIED) continue;

        if (dest->count + 1 > (dest->capacity * 3) / 4) {
            size_t new_capacity = dest->capacity < 8 ? 8 : dest->capacity * 2;
            adjust_capacity(dest, new_capacity);
        }

        ElyEntry* slot = find_entry(dest->entries, dest->capacity, entry->key);
        if (slot->status != ELY_ENTRY_OCCUPIED) {
            dest->count++;
        }

        *slot = *entry;
    }
}
