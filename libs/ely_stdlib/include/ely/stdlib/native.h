#ifndef ELY_STDLIB_NATIVE_H
#define ELY_STDLIB_NATIVE_H

#include <ely/core/value.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Computes the 32-bit FNV-1a hash of a null-terminated string.
 *
 * @param str Null-terminated string to hash.
 * @return 32-bit unsigned hash value.
 */
ELY_NODISCARD uint32_t ely_hash_string(const char* str);

/**
 * Function pointer signature for both internal and foreign native standard functions.
 *
 * Receives the count of arguments passed on the VM stack and a contiguous pointer
 * to the evaluated argument Values, returning an IEEE-754 NaN-boxed ElyValue.
 */
typedef ElyValue (*ElyNativeFn)(uint8_t argc, const ElyValue* args);

/**
 * Represents the occupancy state of a hash table bucket entry.
 */
typedef enum ElyEntryStatus {
    /// Bucket is unused and contains no data.
    ELY_ENTRY_EMPTY,
    /// Bucket holds an active key-value pair.
    ELY_ENTRY_OCCUPIED,
    /// Bucket was previously occupied but has been deleted (probing continues).
    ELY_ENTRY_TOMBSTONE
} ElyEntryStatus;

/**
 * Key-value pair entry stored inside an open-addressing hash table.
 *
 * Uses a 32-bit precomputed hash key and a multi-purpose union payload,
 * ensuring seamless portability across both 32-bit (e.g. ARMv7) and 64-bit architectures.
 */
typedef struct ElyEntry {
    /// 32-bit hash key (e.g. FNV-1a hash of the identifier name).
    uint32_t key;
    /// Entry's current occupancy status.
    ElyEntryStatus status;
    /// Multi-purpose union holding the payload data without pointer-to-integer casts.
    union {
        /// NaN-boxed language value (used for global variables and instance fields).
        ElyValue value;
        /// Function pointer to a native callable (used for native functions table).
        ElyNativeFn native;
        /// Heap or method identifier index (used for class method dispatch tables).
        size_t id;
    };
} ElyEntry;

/**
 * Open-addressing hash table used across the runtime for symbols, globals, and properties.
 */
typedef struct ElyTable {
    /// Dynamically allocated contiguous array of hash table entries.
    ElyEntry* entries;
    /// Number of active key-value pairs currently stored in the table.
    size_t count;
    /// Total capacity of the allocated entries buffer.
    size_t capacity;
} ElyTable;

/**
 * Initialises an empty hash table with zero capacity and null entries buffer.
 *
 * @param table Pointer to the ElyTable to initialise.
 */
void ely_table_init(ElyTable* table);

/**
 * Releases the dynamically allocated entries buffer and re-initialises the table to zero.
 *
 * @param table Pointer to the ElyTable to deallocate.
 */
void ely_table_free(ElyTable* table);

/**
 * Resets all entries in the table to empty without freeing the allocated buffer.
 *
 * @param table Pointer to the ElyTable to clear.
 */
void ely_table_clear(ElyTable* table);

/**
 * Searches the hash table for an occupied entry matching the given string key.
 *
 * Computes the hash internally using ely_hash_string.
 *
 * @param table Pointer to the hash table to search.
 * @param key Null-terminated string key.
 * @return Mutable pointer to the matching entry if found, NULL otherwise.
 */
ELY_NODISCARD ElyEntry* ely_table_find(const ElyTable* table, const char* key);

/**
 * Searches the hash table for an occupied entry matching the given precomputed 32-bit hash.
 *
 * @param table Pointer to the hash table to search.
 * @param hash 32-bit hash key.
 * @return Mutable pointer to the matching entry if found, NULL otherwise.
 */
ELY_NODISCARD ElyEntry* ely_table_find_by_hash(const ElyTable* table, uint32_t hash);

/**
 * Inserts or updates an entry associating the string key with an ElyValue.
 *
 * @param table Pointer to the hash table.
 * @param key Null-terminated string key.
 * @param value NaN-boxed language value to associate.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
bool ely_table_set_value(ElyTable* table, const char* key, ElyValue value);

/**
 * Inserts or updates an entry associating the precomputed 32-bit hash with an ElyValue.
 *
 * @param table Pointer to the hash table.
 * @param hash 32-bit hash key.
 * @param value NaN-boxed language value to associate.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
bool ely_table_set_value_by_hash(ElyTable* table, uint32_t hash, ElyValue value);

/**
 * Inserts or updates an entry associating the string key with a native function pointer.
 *
 * @param table Pointer to the hash table.
 * @param key Null-terminated string key.
 * @param native Function pointer to the native callable.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
ELY_NODISCARD bool ely_table_set_native(ElyTable* table, const char* key, ElyNativeFn native);

/**
 * Inserts or updates an entry associating the precomputed 32-bit hash with a native function pointer.
 *
 * @param table Pointer to the hash table.
 * @param hash 32-bit hash key.
 * @param native Function pointer to the native callable.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
ELY_NODISCARD bool ely_table_set_native_by_hash(ElyTable* table, uint32_t hash, ElyNativeFn native);

/**
 * Inserts or updates an entry associating the string key with an integer or method identifier.
 *
 * @param table Pointer to the hash table.
 * @param key Null-terminated string key.
 * @param id Integer identifier index.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
ELY_NODISCARD bool ely_table_set_id(ElyTable* table, const char* key, size_t id);

/**
 * Inserts or updates an entry associating the precomputed 32-bit hash with an integer or method identifier.
 *
 * @param table Pointer to the hash table.
 * @param hash 32-bit hash key.
 * @param id Integer identifier index.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
ELY_NODISCARD bool ely_table_set_id_by_hash(ElyTable* table, uint32_t hash, size_t id);

/**
 * Removes the entry matching the given string key by replacing it with a tombstone.
 *
 * @param table Pointer to the hash table.
 * @param key Null-terminated string key to remove.
 * @return True if the key was found and removed, false otherwise.
 */
ELY_NODISCARD bool ely_table_delete(ElyTable* table, const char* key);

/**
 * Removes the entry matching the given precomputed 32-bit hash by replacing it with a tombstone.
 *
 * @param table Pointer to the hash table.
 * @param hash 32-bit hash key to remove.
 * @return True if the key was found and removed, false otherwise.
 */
ELY_NODISCARD bool ely_table_delete_by_hash(ElyTable* table, uint32_t hash);

/**
 * Copies all active, occupied entries from the source table into the destination table.
 *
 * Used for class method inheritance and module exports.
 *
 * @param dest Destination hash table.
 * @param src Source hash table.
 */
void ely_table_add_all(ElyTable* dest, const ElyTable* src);

#endif // ELY_STDLIB_NATIVE_H



