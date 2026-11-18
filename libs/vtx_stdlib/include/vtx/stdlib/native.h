#ifndef VTX_STDLIB_NATIVE_H
#define VTX_STDLIB_NATIVE_H

#include <vtx/core/value.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Computes the 32-bit FNV-1a hash of a null-terminated string.
 *
 * @param str Null-terminated string to hash.
 * @return 32-bit unsigned hash value.
 */
VTX_NODISCARD uint32_t vtx_hash_string(const char* str);

/**
 * Function pointer signature for both internal and foreign native standard functions.
 *
 * Receives the count of arguments passed on the VM stack and a contiguous pointer
 * to the evaluated argument Values, returning an IEEE-754 NaN-boxed VtxValue.
 */
typedef VtxValue (*VtxNativeFn)(uint8_t argc, const VtxValue* args);

/**
 * Represents the occupancy state of a hash table bucket entry.
 */
typedef enum VtxEntryStatus {
    /// Bucket is unused and contains no data.
    VTX_ENTRY_EMPTY,
    /// Bucket holds an active key-value pair.
    VTX_ENTRY_OCCUPIED,
    /// Bucket was previously occupied but has been deleted (probing continues).
    VTX_ENTRY_TOMBSTONE
} VtxEntryStatus;

/**
 * Key-value pair entry stored inside an open-addressing hash table.
 *
 * Uses a 32-bit precomputed hash key and a multi-purpose union payload,
 * ensuring seamless portability across both 32-bit (e.g. ARMv7) and 64-bit architectures.
 */
typedef struct VtxEntry {
    /// 32-bit hash key (e.g. FNV-1a hash of the identifier name).
    uint32_t key;
    /// Entry's current occupancy status.
    VtxEntryStatus status;
    /// Multi-purpose union holding the payload data without pointer-to-integer casts.
    union {
        /// NaN-boxed language value (used for global variables and instance fields).
        VtxValue value;
        /// Function pointer to a native callable (used for native functions table).
        VtxNativeFn native;
        /// Heap or method identifier index (used for class method dispatch tables).
        size_t id;
    };
} VtxEntry;

/**
 * Open-addressing hash table used across the runtime for symbols, globals, and properties.
 */
typedef struct VtxTable {
    /// Dynamically allocated contiguous array of hash table entries.
    VtxEntry* entries;
    /// Number of active key-value pairs currently stored in the table.
    size_t count;
    /// Total capacity of the allocated entries buffer.
    size_t capacity;
} VtxTable;

/**
 * Initialises an empty hash table with zero capacity and null entries buffer.
 *
 * @param table Pointer to the VtxTable to initialise.
 */
void vtx_table_init(VtxTable* table);

/**
 * Releases the dynamically allocated entries buffer and re-initialises the table to zero.
 *
 * @param table Pointer to the VtxTable to deallocate.
 */
void vtx_table_free(VtxTable* table);

/**
 * Resets all entries in the table to empty without freeing the allocated buffer.
 *
 * @param table Pointer to the VtxTable to clear.
 */
void vtx_table_clear(VtxTable* table);

/**
 * Searches the hash table for an occupied entry matching the given string key.
 *
 * Computes the hash internally using vtx_hash_string.
 *
 * @param table Pointer to the hash table to search.
 * @param key Null-terminated string key.
 * @return Mutable pointer to the matching entry if found, NULL otherwise.
 */
VTX_NODISCARD VtxEntry* vtx_table_find(const VtxTable* table, const char* key);

/**
 * Searches the hash table for an occupied entry matching the given precomputed 32-bit hash.
 *
 * @param table Pointer to the hash table to search.
 * @param hash 32-bit hash key.
 * @return Mutable pointer to the matching entry if found, NULL otherwise.
 */
VTX_NODISCARD VtxEntry* vtx_table_find_by_hash(const VtxTable* table, uint32_t hash);

/**
 * Inserts or updates an entry associating the string key with an VtxValue.
 *
 * @param table Pointer to the hash table.
 * @param key Null-terminated string key.
 * @param value NaN-boxed language value to associate.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
bool vtx_table_set_value(VtxTable* table, const char* key, VtxValue value);

/**
 * Inserts or updates an entry associating the precomputed 32-bit hash with an VtxValue.
 *
 * @param table Pointer to the hash table.
 * @param hash 32-bit hash key.
 * @param value NaN-boxed language value to associate.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
bool vtx_table_set_value_by_hash(VtxTable* table, uint32_t hash, VtxValue value);

/**
 * Inserts or updates an entry associating the string key with a native function pointer.
 *
 * @param table Pointer to the hash table.
 * @param key Null-terminated string key.
 * @param native Function pointer to the native callable.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
VTX_NODISCARD bool vtx_table_set_native(VtxTable* table, const char* key, VtxNativeFn native);

/**
 * Inserts or updates an entry associating the precomputed 32-bit hash with a native function pointer.
 *
 * @param table Pointer to the hash table.
 * @param hash 32-bit hash key.
 * @param native Function pointer to the native callable.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
VTX_NODISCARD bool vtx_table_set_native_by_hash(VtxTable* table, uint32_t hash, VtxNativeFn native);

/**
 * Inserts or updates an entry associating the string key with an integer or method identifier.
 *
 * @param table Pointer to the hash table.
 * @param key Null-terminated string key.
 * @param id Integer identifier index.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
VTX_NODISCARD bool vtx_table_set_id(VtxTable* table, const char* key, size_t id);

/**
 * Inserts or updates an entry associating the precomputed 32-bit hash with an integer or method identifier.
 *
 * @param table Pointer to the hash table.
 * @param hash 32-bit hash key.
 * @param id Integer identifier index.
 * @return True if a new key was inserted, false if an existing key was updated.
 */
VTX_NODISCARD bool vtx_table_set_id_by_hash(VtxTable* table, uint32_t hash, size_t id);

/**
 * Removes the entry matching the given string key by replacing it with a tombstone.
 *
 * @param table Pointer to the hash table.
 * @param key Null-terminated string key to remove.
 * @return True if the key was found and removed, false otherwise.
 */
VTX_NODISCARD bool vtx_table_delete(VtxTable* table, const char* key);

/**
 * Removes the entry matching the given precomputed 32-bit hash by replacing it with a tombstone.
 *
 * @param table Pointer to the hash table.
 * @param hash 32-bit hash key to remove.
 * @return True if the key was found and removed, false otherwise.
 */
VTX_NODISCARD bool vtx_table_delete_by_hash(VtxTable* table, uint32_t hash);

/**
 * Copies all active, occupied entries from the source table into the destination table.
 *
 * Used for class method inheritance and module exports.
 *
 * @param dest Destination hash table.
 * @param src Source hash table.
 */
void vtx_table_add_all(VtxTable* dest, const VtxTable* src);

#endif // VTX_STDLIB_NATIVE_H



