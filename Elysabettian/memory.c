//
//  memory.c
//  Elysabettian
//
//  Created by Simone Rolando on 06/07/21.
//

#include "memory.h"
#include "vm.h"
#include "compiler.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#include <stdlib.h>

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, int old_size, int new_size)
{
    vm.bytes_allocated += new_size - old_size;
    if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
        gc_collect();
#endif
        if (vm.bytes_allocated > vm.next_gc)
            gc_collect();
    }
    
    void* result = realloc(pointer, new_size);
    if (result == NULL) exit(EXIT_FAILURE);
    return result;
}

void mark_object(Object* object)
{
    if (object == NULL) return;
    if (object->is_marked) return;
    
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    print_value(OBJECT_VAL(object));
    printf("\n");
#endif
    
    object->is_marked = true;
    
    if (vm.gray_capacity < vm.gray_count + 1) {
        vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
        vm.gray_stack = (Object**)realloc(vm.gray_stack, sizeof(Object) * vm.gray_capacity);
        if (vm.gray_stack == NULL) exit(EXIT_FAILURE);
    }
    vm.gray_stack[vm.gray_count++] = object;
}

void mark_value(Value value)
{
    if (IS_OBJECT(value)) mark_object(AS_OBJECT(value));
}

static void mark_array(ValueArray* array)
{
    int i;
    for (i = 0; i < array->count; ++i)
        mark_value(array->values[i]);
}

static void blacken_object(Object* object)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    print_value(OBJECT_VAL(object));
    printf("\n");
#endif
    
    switch (object->type) {
        case OBJ_BOUND_FUN: {
            ObjBoundFun* bound = (ObjBoundFun*)object;
            mark_value(bound->receiver);
            mark_object((Object*)bound->function);
            break;
        }
        case OBJ_CLASS: {
            ObjClass* class = (ObjClass*)object;
            mark_object((Object*)class->name);
            mark_table(&class->member_functions);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            mark_object((Object*)closure->function);
            int i;
            for (i = 0; i < closure->upvalue_count; ++i)
                mark_object((Object*)closure->upvalues[i]);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            mark_object((Object*)function->name);
            mark_array(&function->chunk.constants);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            mark_object((Object*)instance->class);
            mark_table(&instance->fields);
            break;
        }
        case OBJ_UPVALUE: {
            mark_value(((ObjUpvalue*)object)->closed);
            break;
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
        }
    }
}

static void free_object(Object* object)
{
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif
    switch (object->type) {
        case OBJ_BOUND_FUN:
            FREE(ObjBoundFun, object);
            break;
        case OBJ_CLASS: {
            ObjClass* class = (ObjClass*)object;
            free_table(&class->member_functions);
            FREE(ObjClass, object);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalue_count);
            FREE(ObjClosure, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            free_chunk(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            free_table(&instance->fields);
            FREE(ObjInstance, object);
            break;
        }
        case OBJ_NATIVE:
            FREE(ObjNative, object);
            break;
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_UPVALUE:
            FREE(ObjUpvalue, object);
            break;
    }
}

static void mark_roots()
{
    Value* slot;
    for (slot = vm.stack; slot < vm.stack_top; slot++)
        mark_value(*slot);
    
    int i;
    for (i = 0; i < vm.frame_count; ++i)
        mark_object((Object*)vm.call_frames[i].closure);
    
    ObjUpvalue* upvalue;
    for (upvalue = vm.open_upvalues; upvalue != NULL; upvalue = upvalue->next)
        mark_object((Object*)upvalue);
    
    mark_table(&vm.globals);
    
    mark_compiler_roots();
    
    mark_object((Object*)vm.init_strings);
}

static void trace_references()
{
    while (vm.gray_count > 0) {
        Object* object = vm.gray_stack[--vm.gray_count];
        blacken_object(object);
    }
}

static void sweep()
{
    Object* previous = NULL;
    Object* object = vm.objects;
    while (object != NULL) {
        if (object->is_marked) {
            object->is_marked = false;
            previous = object;
            object = object->next;
        } else {
            Object* unreached = object;
            object = object->next;
            if (previous != NULL)
                previous->next = object;
            else
                vm.objects = object;
            free_object(unreached);
        }
    }
}

void gc_collect()
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    int before = vm.bytes_allocated;
#endif
    
    mark_roots();
    trace_references();
    table_remove_white(&vm.strings);
    sweep();
    
    vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;
    
#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("  collected %d bytes (from %d to %d) next at %d\n",
           before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif
}

void free_objects(void)
{
    Object* object = vm.objects;
    while (object != NULL) {
        Object* next = object->next;
        free_object(object);
        object = next;
    }
    free(vm.gray_stack);
}
