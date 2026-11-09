#include <ely/vm/vm.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void ely_vm_init(ElyVM* vm) {
    assert(vm != NULL && "VM pointer is NULL.");

    // Reset counters
    vm->frame_count = 0;
    vm->stack_count = 0;
    vm->open_upvalues_count = 0;

    // Set initial capacity
    vm->stack = (ElyValue*)malloc(sizeof(ElyValue) * ELY_VM_STACK_INITIAL_CAPACITY);
    assert(vm->stack != NULL && "Out of memory, cannot allocate VM stack.");
    vm->stack_capacity = ELY_VM_STACK_INITIAL_CAPACITY;

    // Set open upvalues to NULL
    vm->open_upvalues = NULL;
    vm->open_upvalues_capacity = 0;

    ely_heap_init(&vm->heap);

    // Initialise tables
    ely_table_init(&vm->globals);
    ely_table_init(&vm->natives);

    // Setup internal flags
    vm->jit_enabled = true;
    vm->dump_asm = false;
    vm->dump_ir = false;
}

void ely_vm_free(ElyVM* vm) {
    assert(vm != NULL && "VM pointer is NULL.");
    free(vm->stack);
    ely_heap_free(&vm->heap);
    free(vm->open_upvalues);
    ely_table_free(&vm->globals);
    ely_table_free(&vm->natives);
    vm->frame_count = 0;
}

ElyInterpretResult ely_vm_interpret_from_source(ElyVM* vm, const char* source) {
    (void)vm;
    (void)source;
    return ELY_INTERPRET_OK;
}

ElyInterpretResult ely_vm_interpret(ElyVM* vm, const ElyChunk* chunk) {
    (void)vm;
    (void)chunk;
    return ELY_INTERPRET_OK;
}

void ely_vm_push(ElyVM* vm, ElyValue value) {
    (void)vm;
    (void)value;
}

ElyValue ely_vm_pop(ElyVM* vm) {
    (void)vm;
    return ely_value_null();
}

ElyValue ely_vm_peek(const ElyVM* vm, size_t distance) {
    (void)vm;
    (void)distance;
    return ely_value_null();
}

void ely_vm_reset_stack(ElyVM* vm) {
    assert(vm != NULL && "VM pointer is NULL.");
    
    vm->stack_count = 0;
    vm->frame_count = 0;
    vm->open_upvalues_count = 0;
}

ElyValue ely_vm_allocate_object(ElyVM* vm, const ElyHeapObject* obj) {
    (void)vm;
    (void)obj;
    return ely_value_null();
}

const ElyHeapObject* ely_vm_get_object_const(const ElyVM* vm, ElyValue value) {
    (void)vm;
    (void)value;
    return NULL;
}

ElyHeapObject* ely_vm_get_object(ElyVM* vm, ElyValue value) {
    (void)vm;
    (void)value;
    return NULL;
}

void ely_vm_define_native(ElyVM* vm, uint32_t name_id, ElyNativeFn native) {
    (void)vm;
    (void)name_id;
    (void)native;
}
