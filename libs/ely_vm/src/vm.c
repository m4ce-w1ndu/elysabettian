#include <ely/vm/vm.h>
#include <ely/core/opcode.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/**
 * Captures a local variable at the given stack slot into an open upvalue.
 *
 * @param vm Pointer to the active virtual machine.
 * @param location VM stack slot index.
 * @return Heap identifier of the newly created or existing ElyObjUpvalue.
 */
static inline size_t capture_upvalue(ElyVM* vm, size_t location) {
    assert(vm != NULL && "VM pointer is NULL.");

    // Find the upvalue in the array of open upvalues
    for (size_t i = 0; i < vm->open_upvalues_count; ++i) {
        const size_t id = vm->open_upvalues[i];
        const ElyObjUpvalue* up = ely_heap_get_if_const(&vm->heap, id, ELY_HEAP_OBJECT_UPVALUE);
        if (up != NULL && !up->is_closed && up->location == location) {
            return id;
        }
    }

    // If it is not present, create a new one
    ElyHeapObject obj = {
        .type = ELY_HEAP_OBJECT_UPVALUE,
        .upvalue = {
            .location = location,
            .closed = ely_value_null(),
            .is_closed = false
        },
    };

    const size_t new_id = ely_heap_allocate(&vm->heap, &obj);

    // Push it to the VM's buffer
    if (vm->open_upvalues_count + 1 > vm->open_upvalues_capacity) {
        const size_t new_capacity = vm->open_upvalues_capacity < 8 ? 8 : vm->open_upvalues_capacity * 2;
        size_t* new_buffer = (size_t*)realloc(vm->open_upvalues, sizeof(size_t) * new_capacity);
        assert(new_buffer != NULL && "Out of memory, cannot reallocate open upvalues buffer.");
        vm->open_upvalues = new_buffer;
        vm->open_upvalues_capacity = new_capacity;
    }

    // Add it and return the new id
    vm->open_upvalues[vm->open_upvalues_count++] = new_id;
    return new_id;
}

/**
 * Closes all open upvalues pointing to stack slots at or above `slot`.
 *
 * @param vm Pointer to the active virtual machine.
 * @param slot Threshold stack slot offset.
 */
static inline void close_upvalue(ElyVM* vm, size_t slot) {
    assert(vm != NULL && "VM pointer is NULL.");

    size_t write_idx = 0;
    for (size_t i = 0; i < vm->open_upvalues_count; ++i) {
        const size_t id = vm->open_upvalues[i];
        ElyObjUpvalue* up = ely_heap_get_if(&vm->heap, id, ELY_HEAP_OBJECT_UPVALUE);

        if (up != NULL && up->location >= slot) {
            up->closed = vm->stack[up->location];
            up->is_closed = true;
        } else {
            vm->open_upvalues[write_idx++] = id;
        }
    }

    vm->open_upvalues_count = write_idx;
}

/**
 * Reads the current runtime Value referenced by a closure's upvalue.
 *
 * @param vm Read-only pointer to the active virtual machine.
 * @param closure_id Heap identifier of the closure.
 * @param slot Index into the closure's upvalue table.
 * @param out_value Output pointer receiving the evaluated Value on success.
 * @return True if the upvalue was read successfully, false otherwise.
 */
static inline bool get_upvalue_value(const ElyVM* vm, size_t closure_id, size_t slot, ElyValue* out_value) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(out_value != NULL && "Out value pointer is NULL.");

    const ElyObjClosure* closure = ely_heap_get_if_const(&vm->heap, closure_id, ELY_HEAP_OBJECT_CLOSURE);
    if (closure == NULL || slot >= closure->upvalue_count) return false;
    const ElyObjUpvalue* up = ely_heap_get_if_const(&vm->heap, closure->upvalue_ids[slot], ELY_HEAP_OBJECT_UPVALUE);
    if (up == NULL) return false;

    if (up->is_closed) {
        *out_value = up->closed;
    } else {
        *out_value  = vm->stack[up->location];
    }

    return true;
}

/**
 * Mutates the value referenced by a closure's upvalue.
 *
 * @param vm Pointer to the active virtual machine.
 * @param closure_id Heap identifier of the closure.
 * @param slot Index into the closure's upvalue table.
 * @param value New Value to store.
 * @return True on success, false otherwise.
 */
static inline bool set_upvalue_value(ElyVM* vm, size_t closure_id, size_t slot, ElyValue value) {
    assert(vm != NULL && "VM pointer is NULL.");

    const ElyObjClosure* closure = ely_heap_get_if_const(&vm->heap, closure_id, ELY_HEAP_OBJECT_CLOSURE);
    if (closure == NULL || slot >= closure->upvalue_count) return false;
    ElyObjUpvalue* up = ely_heap_get_if(&vm->heap, closure->upvalue_ids[slot], ELY_HEAP_OBJECT_UPVALUE);
    if (up == NULL) return false;

    if (up->is_closed) {
        up->closed = value;
    } else {
        vm->stack[up->location] = value;
    }

    return true;
}

/**
 * Recursively loads string constants and compiled functions onto the heap
 * and links their handles into chunk constants.
 *
 * @param vm Pointer to the active virtual machine.
 * @param chunk Bytecode chunk to load and patch.
 */
static void load_chunk_objects(ElyVM* vm, ElyChunk* chunk) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(chunk != NULL && "Chunk pointer is NULL.");

    ElyValue* string_vals = NULL;
    if (chunk->string_constants_count > 0) {
        string_vals = (ElyValue*)malloc(sizeof(ElyValue) * chunk->string_constants_count);
        assert(string_vals != NULL && "Out of memory, cannot allocate strings temp buffer.");

        for (size_t i = 0; i < chunk->string_constants_count; ++i) {
            const char* s = chunk->string_constants[i];
            const size_t len = strlen(s);
            char* chars = (char*)malloc(len + 1);
            assert(chars != NULL && "Out of memory, cannot allocate string copy.");
            memcpy(chars, s, len + 1);

            const ElyHeapObject obj = {
                .type = ELY_HEAP_OBJECT_STRING,
                .string = {
                    .length = len,
                    .chars = chars,
                    .hash = ely_hash_string(chars),
                },
            };
            string_vals[i] = ely_vm_allocate_object(vm, &obj);
        }
    }

    ElyValue* func_vals = NULL;
    if (chunk->function_constants_count > 0) {
        func_vals = (ElyValue*)malloc(sizeof(ElyValue) * chunk->function_constants_count);
        assert(func_vals != NULL && "Out of memory, cannot allocate functions temp buffer.");

        for (size_t i = 0; i < chunk->function_constants_count; ++i) {
            ElyFunctionDescriptor* f = &chunk->function_constants[i];
            if (f->chunk != NULL) {
                load_chunk_objects(vm, f->chunk);
            }

            const ElyHeapObject obj = {
                .type = ELY_HEAP_OBJECT_FUNCTION,
                .function = {
                    .arity = f->arity,
                    .upvalue_count = 0,
                    .chunk = f->chunk,
                    .name = f->name,
                    .invocation_count = 0,
                    .jit_entry = NULL,
                },
            };
            // Transfer ownership of chunk and name to the heap object.
            f->chunk = NULL;
            f->name = NULL;

            func_vals[i] = ely_vm_allocate_object(vm, &obj);
        }
    }

    size_t ip = 0;
    while (ip < chunk->code_count) {
        const ElyOpcode opcode = (ElyOpcode)chunk->code[ip++];

        switch (opcode) {
            case ELY_OP_CONSTANT: {
                const size_t const_idx = (size_t)chunk->code[ip++];
                if (const_idx < chunk->constants_count && ely_value_is_obj(chunk->constants[const_idx])) {
                    const size_t orig_idx = (size_t)ely_value_as_obj(chunk->constants[const_idx]);
                    if (orig_idx < chunk->string_constants_count) {
                        chunk->constants[const_idx] = string_vals[orig_idx];
                    }
                }
                break;
            }

            case ELY_OP_CLOSURE: {
                const size_t const_idx = (size_t)chunk->code[ip++];
                if (const_idx < chunk->constants_count && ely_value_is_obj(chunk->constants[const_idx])) {
                    const size_t orig_idx = (size_t)ely_value_as_obj(chunk->constants[const_idx]);
                    if (orig_idx < chunk->function_constants_count) {
                        chunk->constants[const_idx] = func_vals[orig_idx];
                    }
                }
                break;
            }

            case ELY_OP_GETLOCAL:
            case ELY_OP_SETLOCAL:
            case ELY_OP_GETGLOBAL:
            case ELY_OP_DEFINEGLOBAL:
            case ELY_OP_SETGLOBAL:
            case ELY_OP_CALL:
            case ELY_OP_CLASS:
            case ELY_OP_GETPROPERTY:
            case ELY_OP_SETPROPERTY:
            case ELY_OP_METHOD:
            case ELY_OP_ARRAYBUILD:
            case ELY_OP_IMPORT:
                ip += 1;
                break;

            case ELY_OP_INVOKE:
            case ELY_OP_SUPERINVOKE:
            case ELY_OP_JUMP:
            case ELY_OP_JUMPZERO:
            case ELY_OP_LOOP:
                ip += 2;
                break;

            default:
                break;
        }
    }

    free(string_vals);
    free(func_vals);
}

/**
 * Formats a Value into a human-readable display string representation.
 *
 * @param vm Read-only pointer to the active virtual machine.
 * @param value Target Value to format.
 * @param buf Destination character buffer.
 * @param buf_size Size of the destination buffer in bytes.
 */
static inline void format_value(const ElyVM* vm, ElyValue value, char* buf, size_t buf_size);

/**
 * Reports a formatted runtime error with stack trace and resets the VM stack.
 *
 * @param vm Pointer to the active virtual machine.
 * @param fmt Printf-style format string.
 */
static inline void runtime_error(ElyVM* vm, const char* fmt, ...);

/**
 * Core bytecode execution and instruction dispatch loop.
 *
 * @param vm Pointer to the active virtual machine.
 * @return Execution result of the top-level script or active frame.
 */
static ElyInterpretResult run(ElyVM* vm);


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
    assert(vm != NULL && "VM pointer is NULL.");

    if (vm->stack_count + 1 > vm->stack_capacity) {
        size_t new_capacity = vm->stack_capacity < 8 ? 8 : vm->stack_capacity * 2;
        ElyValue* new_stack = (ElyValue*)realloc(vm->stack, sizeof(ElyValue) * new_capacity);
        assert(new_stack != NULL && "Out of memory, cannot reallocate VM stack.");
        vm->stack = new_stack;
        vm->stack_capacity = new_capacity;
    }

    vm->stack[vm->stack_count++] = value;
}

ElyValue ely_vm_pop(ElyVM* vm) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(vm->stack_count > 0 && "VM stack is empty.");

    return vm->stack[--vm->stack_count];
}

ElyValue ely_vm_peek(const ElyVM* vm, size_t distance) {
    assert(vm != NULL && "VM pointer is NULL.");

    if (distance >= vm->stack_count) return ely_value_null();
    return vm->stack[vm->stack_count - 1 - distance];
}

void ely_vm_reset_stack(ElyVM* vm) {
    assert(vm != NULL && "VM pointer is NULL.");
    
    vm->stack_count = 0;
    vm->frame_count = 0;
    vm->open_upvalues_count = 0;
}

ElyValue ely_vm_allocate_object(ElyVM* vm, const ElyHeapObject* obj) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(obj != NULL && "Object pointer is NULL.");
    
    const size_t obj_id = ely_heap_allocate(&vm->heap, obj);
    return ely_value_obj((uint64_t)obj_id);
}

const ElyHeapObject* ely_vm_get_object_const(const ElyVM* vm, ElyValue value) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(ely_value_is_obj(value) && "Value is not an object.");
    const uint64_t obj = ely_value_as_obj(value);
    return ely_heap_get_const(&vm->heap, (size_t)obj);
}

ElyHeapObject* ely_vm_get_object(ElyVM* vm, ElyValue value) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(ely_value_is_obj(value) && "Value is not an object.");
    const uint64_t obj = ely_value_as_obj(value);
    return ely_heap_get(&vm->heap, (size_t)obj);
}

void ely_vm_define_native(ElyVM* vm, uint32_t name_id, ElyNativeFn native) {
    assert(vm != NULL && "VM pointer is NULL.");
    const bool ok = ely_table_set_native_by_hash(&vm->natives, name_id, native);
    (void)ok;
}
