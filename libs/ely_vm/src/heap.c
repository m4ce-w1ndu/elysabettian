#include <ely/vm/heap.h>
#include <ely/vm/heap_object.h>

#include <assert.h>
#include <stdlib.h>

/**
 * Releases dynamic heap memory held internally by a specific heap object variant.
 *
 * @param object Pointer to the heap object whose internal resources should be freed.
 */
static void heap_object_free_resources(ElyHeapObject* object) {
    if (object == NULL) return;

    switch (object->type) {
        case ELY_HEAP_OBJECT_STRING:
            free(object->string.chars);
            break;
        case ELY_HEAP_OBJECT_ARRAY:
            free(object->array.values);
            break;
        case ELY_HEAP_OBJECT_FUNCTION:
            if (object->function.chunk != NULL) {
                ely_chunk_free(object->function.chunk);
                free(object->function.chunk);
            }
            free((void*)object->function.name);
            break;
        case ELY_HEAP_OBJECT_CLOSURE:
            free(object->closure.upvalue_ids);
            break;
        case ELY_HEAP_OBJECT_CLASS:
            free(object->class_obj.methods);
            break;
        case ELY_HEAP_OBJECT_INSTANCE:
            free(object->instance.fields);
            break;
        case ELY_HEAP_OBJECT_UPVALUE:
        case ELY_HEAP_OBJECT_BOUND_METHOD:
            // No additional resources to free for these types.
            break;
        default:
            // Unknown type; do nothing.
            break;
    }
}

void ely_heap_init(ElyHeap* heap) {
    assert(heap != NULL && "Heap pointer is NULL.");
    heap->objects = NULL;
    heap->object_count = 0;
    heap->object_capacity = 0;
}

void ely_heap_free(ElyHeap* heap) {
    assert(heap != NULL && "Heap pointer is NULL.");
    ely_heap_clear(heap);
    free(heap->objects);
    ely_heap_init(heap);
}

ELY_NODISCARD size_t ely_heap_allocate(ElyHeap* heap, const ElyHeapObject* object) {
    assert(heap != NULL && "Heap pointer is NULL.");

    if (heap->object_count + 1 > heap->object_capacity) {
        size_t new_capacity = heap->object_capacity < 8 ? 8 : heap->object_capacity * 2;
        ElyHeapObject* new_objs = (ElyHeapObject*)realloc(heap->objects, sizeof(ElyHeapObject) * new_capacity);
        assert(new_objs != NULL && "Out of memory, cannot reallocate heap objects.");
        heap->objects = new_objs;
        heap->object_capacity = new_capacity;
    }

    memcpy(&heap->objects[heap->object_count++], object, sizeof(struct ElyHeapObject));
    return heap->object_count - 1;
}

ELY_NODISCARD ElyHeapObject* ely_heap_get(ElyHeap* heap, size_t object_id) {
    assert(heap != NULL && "Heap pointer is NULL.");
    assert(object_id < heap->object_count && "Heap object index out of bounds.");
    return &heap->objects[object_id];
}

ELY_NODISCARD const ElyHeapObject* ely_heap_get_const(const ElyHeap* heap, size_t object_id) {
    assert(heap != NULL && "Heap pointer is NULL.");
    assert(object_id < heap->object_count && "Heap object index out of bounds.");
    return &heap->objects[object_id];
}

ELY_NODISCARD void* ely_heap_get_if(ElyHeap* heap, size_t object_id, ElyHeapObjectType expected_type) {
    return (void*)ely_heap_get_if_const(heap, object_id, expected_type);
}

ELY_NODISCARD const void* ely_heap_get_if_const(const ElyHeap* heap, size_t object_id, ElyHeapObjectType expected_type) {
    assert(heap != NULL && "Heap pointer is NULL.");
    if (object_id >= heap->object_count) return NULL;
    
    ElyHeapObject* ptr = &heap->objects[object_id];
    if (ptr->type != expected_type) return NULL;

    switch (ptr->type) {
        case ELY_HEAP_OBJECT_STRING:
            return &ptr->string;
        case ELY_HEAP_OBJECT_ARRAY:
            return &ptr->array;
        case ELY_HEAP_OBJECT_FUNCTION:
            return &ptr->function;
        case ELY_HEAP_OBJECT_CLOSURE:
            return &ptr->closure;
        case ELY_HEAP_OBJECT_UPVALUE:
            return &ptr->upvalue;
        case ELY_HEAP_OBJECT_CLASS:
            return &ptr->class_obj;
        case ELY_HEAP_OBJECT_INSTANCE:
            return &ptr->instance;
        case ELY_HEAP_OBJECT_BOUND_METHOD:
            return &ptr->bound_method;
        default: return NULL;
    }
}

void ely_heap_clear(ElyHeap* heap) {
    if (heap == NULL) return;

    for (size_t i = 0; i < heap->object_count; ++i) {
        heap_object_free_resources(&heap->objects[i]);
    }
    heap->object_count = 0;
}
