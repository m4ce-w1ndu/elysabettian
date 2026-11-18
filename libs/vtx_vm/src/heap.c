#include <vtx/vm/heap.h>
#include <vtx/vm/heap_object.h>

#include <assert.h>
#include <stdlib.h>

/**
 * Releases dynamic heap memory held internally by a specific heap object variant.
 *
 * @param object Pointer to the heap object whose internal resources should be freed.
 */
static void heap_object_free_resources(VtxHeapObject* object) {
    if (object == NULL) return;

    switch (object->type) {
        case VTX_HEAP_OBJECT_STRING:
            free(object->string.chars);
            break;
        case VTX_HEAP_OBJECT_ARRAY:
            free(object->array.values);
            break;
        case VTX_HEAP_OBJECT_FUNCTION:
            if (object->function.chunk != NULL) {
                vtx_chunk_free(object->function.chunk);
                free(object->function.chunk);
            }
            free((void*)object->function.name);
            break;
        case VTX_HEAP_OBJECT_CLOSURE:
            free(object->closure.upvalue_ids);
            break;
        case VTX_HEAP_OBJECT_CLASS:
            free(object->class_obj.methods);
            break;
        case VTX_HEAP_OBJECT_INSTANCE:
            free(object->instance.fields);
            break;
        case VTX_HEAP_OBJECT_UPVALUE:
        case VTX_HEAP_OBJECT_BOUND_METHOD:
            // No additional resources to free for these types.
            break;
        default:
            // Unknown type; do nothing.
            break;
    }
}

void vtx_heap_init(VtxHeap* heap) {
    assert(heap != NULL && "Heap pointer is NULL.");
    heap->objects = NULL;
    heap->object_count = 0;
    heap->object_capacity = 0;
}

void vtx_heap_free(VtxHeap* heap) {
    assert(heap != NULL && "Heap pointer is NULL.");
    vtx_heap_clear(heap);
    free(heap->objects);
    vtx_heap_init(heap);
}

VTX_NODISCARD size_t vtx_heap_allocate(VtxHeap* heap, const VtxHeapObject* object) {
    assert(heap != NULL && "Heap pointer is NULL.");

    if (heap->object_count + 1 > heap->object_capacity) {
        size_t new_capacity = heap->object_capacity < 8 ? 8 : heap->object_capacity * 2;
        VtxHeapObject* new_objs = (VtxHeapObject*)realloc(heap->objects, sizeof(VtxHeapObject) * new_capacity);
        assert(new_objs != NULL && "Out of memory, cannot reallocate heap objects.");
        heap->objects = new_objs;
        heap->object_capacity = new_capacity;
    }

    memcpy(&heap->objects[heap->object_count++], object, sizeof(struct VtxHeapObject));
    return heap->object_count - 1;
}

VTX_NODISCARD VtxHeapObject* vtx_heap_get(VtxHeap* heap, size_t object_id) {
    assert(heap != NULL && "Heap pointer is NULL.");
    assert(object_id < heap->object_count && "Heap object index out of bounds.");
    return &heap->objects[object_id];
}

VTX_NODISCARD const VtxHeapObject* vtx_heap_get_const(const VtxHeap* heap, size_t object_id) {
    assert(heap != NULL && "Heap pointer is NULL.");
    assert(object_id < heap->object_count && "Heap object index out of bounds.");
    return &heap->objects[object_id];
}

VTX_NODISCARD void* vtx_heap_get_if(VtxHeap* heap, size_t object_id, VtxHeapObjectType expected_type) {
    return (void*)vtx_heap_get_if_const(heap, object_id, expected_type);
}

VTX_NODISCARD const void* vtx_heap_get_if_const(const VtxHeap* heap, size_t object_id, VtxHeapObjectType expected_type) {
    assert(heap != NULL && "Heap pointer is NULL.");
    if (object_id >= heap->object_count) return NULL;
    
    VtxHeapObject* ptr = &heap->objects[object_id];
    if (ptr->type != expected_type) return NULL;

    switch (ptr->type) {
        case VTX_HEAP_OBJECT_STRING:
            return &ptr->string;
        case VTX_HEAP_OBJECT_ARRAY:
            return &ptr->array;
        case VTX_HEAP_OBJECT_FUNCTION:
            return &ptr->function;
        case VTX_HEAP_OBJECT_CLOSURE:
            return &ptr->closure;
        case VTX_HEAP_OBJECT_UPVALUE:
            return &ptr->upvalue;
        case VTX_HEAP_OBJECT_CLASS:
            return &ptr->class_obj;
        case VTX_HEAP_OBJECT_INSTANCE:
            return &ptr->instance;
        case VTX_HEAP_OBJECT_BOUND_METHOD:
            return &ptr->bound_method;
        default: return NULL;
    }
}

void vtx_heap_clear(VtxHeap* heap) {
    if (heap == NULL) return;

    for (size_t i = 0; i < heap->object_count; ++i) {
        heap_object_free_resources(&heap->objects[i]);
    }
    heap->object_count = 0;
}
