//
//  memory.h
//  Elysabettian
//
//  Created by Simone Rolando on 06/07/21.
//

#ifndef memory_h
#define memory_h

#include "common.h"
#include "object.h"

/* Allocate new memory */
#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

/* Free previously allocated memory */
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

/* Grow capacity parameter */
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

/* Grow a preallocated array */
#define GROW_ARRAY(type, pointer, old_count, new_count) \
    (type*)reallocate(pointer, sizeof(type) * (old_count), \
            sizeof(type) * (new_count))

/* Free a preallocated array */
#define FREE_ARRAY(type, pointer, old_count) \
    reallocate(pointer, sizeof(type) * (old_count), 0)

/* Perform memory allocation operations */
void* reallocate(void* pointer, int old_size, int new_size);

/* System garbage collector functions */
/* Mark an object as garbage */
void mark_object(Object* object);

/* Mark value as garbage */
void mark_value(Value value);

/* Perform garbage collection */
void gc_collect(void);

/* Perform objects freeing */
void free_objects(void);

#endif /* memory_h */
