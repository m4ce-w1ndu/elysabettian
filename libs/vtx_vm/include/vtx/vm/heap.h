#ifndef VTX_VM_HEAP_H
#define VTX_VM_HEAP_H

#include <vtx/core/value.h>
#include "heap_object.h"

#include <stddef.h>

/**
 * Manages dynamically allocated runtime objects for the virtual machine.
 *
 * Encapsulates object lifecycle, provides type-safe accessors via `std::variant`,
 * and assigns unique 48-bit heap identifiers compatible with IEEE-754 NaN-boxing.
 */
typedef struct VtxHeap {
    /// Storage container for all heap-allocated object variants.
    VtxHeapObject* objects;
    /// Current number of allocated objects in the heap.
    size_t object_count;
    /// Allocated capacity of the objects array.
    size_t object_capacity;
} VtxHeap;

/**
 * Initialises an empty object heap with zero capacity and null storage.
 *
 * @param heap Pointer to the VtxHeap structure to initialise.
 */
void vtx_heap_init(VtxHeap* heap);

/**
 * Releases all resources held by allocated heap objects and frees the underlying storage buffer.
 *
 * Re-initialises the heap structure to zero capacity and null pointers upon completion.
 *
 * @param heap Pointer to the VtxHeap structure to deallocate.
 */
void vtx_heap_free(VtxHeap* heap);

/**
 * Emplaces a new object into the heap and returns its unique 0-based identifier.
 *
 * Dynamically resizes the underlying objects array if capacity is exceeded.
 *
 * @param heap Pointer to the active object heap.
 * @param object Pointer of the tagged union object to store.
 * @return 0-based index identifying the allocated object in the heap.
 */
VTX_NODISCARD size_t vtx_heap_allocate(VtxHeap* heap, const VtxHeapObject* object);

/**
 * Retrieves a mutable pointer to the heap object with the given identifier index.
 *
 * Asserts that the identifier index is strictly within the allocated bounds.
 *
 * @param heap Pointer to the active object heap.
 * @param object_id 0-based index of the target object.
 * @return Pointer to the requested VtxHeapObject.
 */
VTX_NODISCARD VtxHeapObject* vtx_heap_get(VtxHeap* heap, size_t object_id);

/**
 * Retrieves an immutable read-only pointer to the heap object with the given identifier index.
 *
 * Asserts that the identifier index is strictly within the allocated bounds.
 *
 * @param heap Read-only pointer to the active object heap.
 * @param object_id 0-based index of the target object.
 * @return Read-only pointer to the requested VtxHeapObject.
 */
VTX_NODISCARD const VtxHeapObject* vtx_heap_get_const(const VtxHeap* heap, size_t object_id);

/**
 * Accesses a heap object as a specific concrete variant type.
 *
 * Verifies bounds and type compatibility. If the object matches the expected type,
 * returns a pointer directly to the concrete object payload; returns NULL otherwise.
 *
 * @param heap Pointer to the active object heap.
 * @param object_id 0-based index of the target object.
 * @param expected_type Discriminator of the expected object type.
 * @return Pointer to the concrete payload struct, or NULL if out of bounds or type mismatch.
 */
VTX_NODISCARD void* vtx_heap_get_if(VtxHeap* heap, size_t object_id, VtxHeapObjectType expected_type);

/**
 * Accesses a heap object as a specific concrete variant type (read-only).
 *
 * Verifies bounds and type compatibility. If the object matches the expected type,
 * returns a const pointer directly to the concrete object payload; returns NULL otherwise.
 *
 * @param heap Read-only pointer to the active object heap.
 * @param object_id 0-based index of the target object.
 * @param expected_type Discriminator of the expected object type.
 * @return Read-only pointer to the concrete payload struct, or NULL if out of bounds or type mismatch.
 */
VTX_NODISCARD const void* vtx_heap_get_if_const(const VtxHeap* heap, size_t object_id, VtxHeapObjectType expected_type);

/**
 * Releases the internal resources of all allocated objects while retaining allocated buffer capacity.
 *
 * Resets object count to zero without freeing the underlying container buffer.
 *
 * @param heap Pointer to the active object heap.
 */
void vtx_heap_clear(VtxHeap* heap);

#endif // VTX_VM_HEAP_H
