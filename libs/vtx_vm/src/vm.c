#include <vtx/vm/vm.h>
#include <vtx/core/opcode.h>
#include <vtx/compiler/compiler.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// Includes intrin.h for compiler intrinsics on Windows.
#if defined(_MSC_VER)
#include <intrin.h>
#endif

/**
 * Computes the smallest power of two greater than or equal to `n`, clamped to at least 8.
 *
 * @param n Minimum required size.
 * @return Power-of-two capacity >= `n` (minimum 8).
 */
static inline size_t next_pow2(size_t n) {
    if (n <= 8) return 8;
#if defined(__GNUC__) || defined(__clang__)
    return (size_t)(1ULL << (64 - __builtin_clzll((unsigned long long)(n - 1))));
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
    unsigned long idx;
    _BitScanReverse64(&idx, n - 1);
    return (size_t)(1ULL << (idx + 1));
#else
    uint64_t v = (uint64_t)(n - 1);
    v |= v >> 1;            // Propagate position by 1 (2 bits)
    v |= v >> 2;            // Propagate position by 2 (4 bits)
    v |= v >> 4;            // Propagate position by 4 (8 bits)
    v |= v >> 8;            // Propagate position by 8 (16 bits)
    v |= v >> 16;           // Propagate position by 16 (32 bits)
    v |= v >> 32;           // Propagate position by 32 (64 bits)
    return (size_t)(v + 1);
#endif
}

/**
 * Captures a local variable at the given stack slot into an open upvalue.
 *
 * @param vm Pointer to the active virtual machine.
 * @param location VM stack slot index.
 * @return Heap identifier of the newly created or existing VtxObjUpvalue.
 */
static inline size_t capture_upvalue(VtxVM* vm, size_t location) {
    assert(vm != NULL && "VM pointer is NULL.");

    // Find the upvalue in the array of open upvalues
    for (size_t i = 0; i < vm->open_upvalues_count; ++i) {
        const size_t id = vm->open_upvalues[i];
        const VtxObjUpvalue* up = vtx_heap_get_if_const(&vm->heap, id, VTX_HEAP_OBJECT_UPVALUE);
        if (up != NULL && !up->is_closed && up->location == location) {
            return id;
        }
    }

    // If it is not present, create a new one
    VtxHeapObject obj = {
        .type = VTX_HEAP_OBJECT_UPVALUE,
        .upvalue = {
            .location = location,
            .closed = vtx_value_null(),
            .is_closed = false
        },
    };

    const size_t new_id = vtx_heap_allocate(&vm->heap, &obj);

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
static inline void close_upvalue(VtxVM* vm, size_t slot) {
    assert(vm != NULL && "VM pointer is NULL.");

    size_t write_idx = 0;
    for (size_t i = 0; i < vm->open_upvalues_count; ++i) {
        const size_t id = vm->open_upvalues[i];
        VtxObjUpvalue* up = vtx_heap_get_if(&vm->heap, id, VTX_HEAP_OBJECT_UPVALUE);

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
static inline bool get_upvalue_value(const VtxVM* vm, size_t closure_id, size_t slot, VtxValue* out_value) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(out_value != NULL && "Out value pointer is NULL.");

    const VtxObjClosure* closure = vtx_heap_get_if_const(&vm->heap, closure_id, VTX_HEAP_OBJECT_CLOSURE);
    if (closure == NULL || slot >= closure->upvalue_count) return false;
    const VtxObjUpvalue* up = vtx_heap_get_if_const(&vm->heap, closure->upvalue_ids[slot], VTX_HEAP_OBJECT_UPVALUE);
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
static inline bool set_upvalue_value(VtxVM* vm, size_t closure_id, size_t slot, VtxValue value) {
    assert(vm != NULL && "VM pointer is NULL.");

    const VtxObjClosure* closure = vtx_heap_get_if_const(&vm->heap, closure_id, VTX_HEAP_OBJECT_CLOSURE);
    if (closure == NULL || slot >= closure->upvalue_count) return false;
    VtxObjUpvalue* up = vtx_heap_get_if(&vm->heap, closure->upvalue_ids[slot], VTX_HEAP_OBJECT_UPVALUE);
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
static void load_chunk_objects(VtxVM* vm, VtxChunk* chunk) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(chunk != NULL && "Chunk pointer is NULL.");

    VtxValue* string_vals = NULL;
    if (chunk->string_constants_count > 0) {
        string_vals = (VtxValue*)malloc(sizeof(VtxValue) * chunk->string_constants_count);
        assert(string_vals != NULL && "Out of memory, cannot allocate strings temp buffer.");

        for (size_t i = 0; i < chunk->string_constants_count; ++i) {
            const char* s = chunk->string_constants[i];
            const size_t len = strlen(s);
            char* chars = (char*)malloc(len + 1);
            assert(chars != NULL && "Out of memory, cannot allocate string copy.");
            memcpy(chars, s, len + 1);

            const VtxHeapObject obj = {
                .type = VTX_HEAP_OBJECT_STRING,
                .string = {
                    .length = len,
                    .chars = chars,
                    .hash = vtx_hash_string(chars),
                },
            };
            string_vals[i] = vtx_vm_allocate_object(vm, &obj);
        }
    }

    VtxValue* func_vals = NULL;
    if (chunk->function_constants_count > 0) {
        func_vals = (VtxValue*)malloc(sizeof(VtxValue) * chunk->function_constants_count);
        assert(func_vals != NULL && "Out of memory, cannot allocate functions temp buffer.");

        for (size_t i = 0; i < chunk->function_constants_count; ++i) {
            VtxFunctionDescriptor* f = &chunk->function_constants[i];
            if (f->chunk != NULL) {
                load_chunk_objects(vm, f->chunk);
            }

            const VtxHeapObject obj = {
                .type = VTX_HEAP_OBJECT_FUNCTION,
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

            func_vals[i] = vtx_vm_allocate_object(vm, &obj);
        }
    }

    size_t ip = 0;
    while (ip < chunk->code_count) {
        const VtxOpcode opcode = (VtxOpcode)chunk->code[ip++];

        switch (opcode) {
            case VTX_OP_CONSTANT: {
                const size_t const_idx = (size_t)chunk->code[ip++];
                if (const_idx < chunk->constants_count && vtx_value_is_obj(chunk->constants[const_idx])) {
                    const size_t orig_idx = (size_t)vtx_value_as_obj(chunk->constants[const_idx]);
                    if (orig_idx < chunk->string_constants_count) {
                        chunk->constants[const_idx] = string_vals[orig_idx];
                    }
                }
                break;
            }

            case VTX_OP_CLOSURE: {
                const size_t const_idx = (size_t)chunk->code[ip++];
                if (const_idx < chunk->constants_count && vtx_value_is_obj(chunk->constants[const_idx])) {
                    const size_t orig_idx = (size_t)vtx_value_as_obj(chunk->constants[const_idx]);
                    if (orig_idx < chunk->function_constants_count) {
                        chunk->constants[const_idx] = func_vals[orig_idx];
                    }
                }
                break;
            }

            case VTX_OP_GETLOCAL:
            case VTX_OP_SETLOCAL:
            case VTX_OP_GETGLOBAL:
            case VTX_OP_DEFINEGLOBAL:
            case VTX_OP_SETGLOBAL:
            case VTX_OP_CALL:
            case VTX_OP_CLASS:
            case VTX_OP_GETPROPERTY:
            case VTX_OP_SETPROPERTY:
            case VTX_OP_METHOD:
            case VTX_OP_ARRAYBUILD:
            case VTX_OP_IMPORT:
                ip += 1;
                break;

            case VTX_OP_INVOKE:
            case VTX_OP_SUPERINVOKE:
            case VTX_OP_JUMP:
            case VTX_OP_JUMPZERO:
            case VTX_OP_LOOP:
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
 * Allocates and formats a string using printf-style variadic arguments.
 *
 * Caller is responsible for freeing the returned string.
 *
 * @param out_len Optional output pointer receiving the resulting string length (excluding NUL).
 * @param fmt Printf-style format string.
 * @return Newly heap-allocated, null-terminated formatted string.
 */
static char* format_allocf(size_t* out_len, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    // Determine required size
    int result = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    // Something failed
    if (result < 0) return NULL;

    // Add space for '\0' and allocate
    result += 1;
    char* buf = malloc(sizeof(char) * (size_t)result);
    if (buf == NULL) return NULL;

    // Print the formatted string
    va_start(ap, fmt);
    int bytes = vsnprintf(buf, (size_t)result, fmt, ap);
    va_end(ap);

    if (bytes < 0) {
        free(buf);
        return NULL;
    }

    if (out_len != NULL) *out_len = (size_t)bytes;
    return buf;
}

/**
 * Appends a byte sequence to a dynamically growing string buffer, NUL-terminating the result.
 *
 * @param buf Address of the heap-allocated character buffer pointer (may be reallocated).
 * @param len Address of the current string length (excluding NUL).
 * @param cap Address of the current allocated buffer capacity in bytes.
 * @param data Pointer to the bytes to append.
 * @param data_len Number of bytes to append from `data`.
 */
static void append_bytes(char** buf, size_t* len, size_t* cap, const char* data, size_t data_len) {
    assert(buf != NULL && "Character buffer pointer is NULL.");
    assert(len != NULL && "Character buffer length tracking pointer is NULL.");
    assert(cap != NULL && "Character buffer capacity tracking pointer is NULL.");
    assert(data != NULL && "Data pointer is NULL.");

    // If there is no data to append, just quit.
    if (data_len == 0) return;

    // Find the new size and reallocate.
    const size_t needed = *len + data_len + 1;
    if (needed > *cap) {
        size_t new_cap = next_pow2(needed);
        char* new_buf = realloc(*buf, sizeof(char) * new_cap);
        assert(new_buf != NULL && "Out of memory, cannot reallocate bytes buffer.");
        *buf = new_buf;
        *cap = new_cap;
    }

    // Copy the new data to the end of the buffer and terminate.
    memcpy(*buf + *len, data, data_len);
    *len += data_len;
    (*buf)[*len] = '\0';
}

static char* format_value(const VtxVM* vm, VtxValue value, size_t* out_len);

/**
 * Formats an array heap object into a newly allocated "[elem1, elem2, ...]" string.
 *
 * Caller is responsible for freeing the returned string.
 *
 * @param vm Read-only pointer to the active virtual machine.
 * @param arr Read-only pointer to the array object to format.
 * @param out_len Optional output pointer receiving the resulting string length (excluding NUL).
 * @return Newly heap-allocated, null-terminated string representation of the array.
 */
static char* format_array(const VtxVM* vm, const VtxObjArray* arr, size_t* out_len) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(arr != NULL && "Array pointer is NULL.");

    char* buf = NULL;
    size_t len = 0, cap = 0;

    // Start by creating the array formatted string
    append_bytes(&buf, &len, &cap, "[", 1);

    // Loop over the array
    for (size_t i = 0; i < arr->length; ++i) {
        if (i > 0) append_bytes(&buf, &len, &cap, ", ", 2);

        // Get the element format
        size_t elem_len = 0;
        char* elem = format_value(vm, arr->values[i], &elem_len);

        // Append the element if it's not NULL.
        if (elem == NULL) continue;
        append_bytes(&buf, &len, &cap, elem, elem_len);
        free(elem);
    }

    append_bytes(&buf, &len, &cap, "]", 1);
    if (out_len != NULL) *out_len = len;
    return buf;
}

/**
 * Formats a Value into a newly heap-allocated human-readable display string.
 *
 * Caller is responsible for freeing the returned string.
 *
 * @param vm Read-only pointer to the active virtual machine.
 * @param value Target Value to format.
 * @param out_len Optional output pointer receiving the resulting string length (excluding NUL).
 * @return Newly heap-allocated, null-terminated string representation of the value.
 */
static char* format_value(const VtxVM* vm, VtxValue value, size_t* out_len) {
    assert(vm != NULL && "VM pointer is NULL.");
    
    // Check the type of the value
    if (vtx_value_is_number(value)) {
        double number = vtx_value_as_number(value);
        return format_allocf(out_len, "%g", number);
    } else if (vtx_value_is_null(value)) {
        return format_allocf(out_len, "null");
    } else if (vtx_value_is_bool(value)) {
        bool boolean = vtx_value_as_bool(value);
        return format_allocf(out_len, "%s", boolean ? "true" : "false");
    } else if (vtx_value_is_obj(value)) {
        const VtxHeapObject* obj = vtx_vm_get_object_const(vm, value);
        if (obj == NULL) return format_allocf(out_len, "<unknown>");

        switch (obj->type) {
            case VTX_HEAP_OBJECT_STRING:
                return format_allocf(out_len, "%s", obj->string.chars);
            case VTX_HEAP_OBJECT_ARRAY:
                return format_array(vm, &obj->array, out_len);
            case VTX_HEAP_OBJECT_FUNCTION: {
                const char* name = obj->function.name
                    ? obj->function.name
                    : "script";
                
                return format_allocf(out_len, "<fn %s>", name);
            }
            case VTX_HEAP_OBJECT_CLOSURE:
                return format_allocf(out_len, "<closure>");
            case VTX_HEAP_OBJECT_CLASS:
                return format_allocf(out_len, "<class %s>", obj->class_obj.name
                    ? obj->class_obj.name
                    : "anonymous");
            case VTX_HEAP_OBJECT_INSTANCE:
                return format_allocf(out_len, "<instance of class %zu>", obj->instance.class_id);
            case VTX_HEAP_OBJECT_BOUND_METHOD:
                return format_allocf(out_len, "<bound method>");
            case VTX_HEAP_OBJECT_UPVALUE:
                return format_allocf(out_len, "<upvalue>");
        }
    }

    return format_allocf(out_len, "<unknown>");
}

/**
 * Reports a formatted runtime error with stack trace and resets the VM stack.
 *
 * @param vm Pointer to the active virtual machine.
 * @param fmt Printf-style format string.
 */
static void runtime_error(VtxVM* vm, const char* fmt, ...) {
    assert(vm != NULL && "VM pointer is NULL.");

    // Handle variadic args
    va_list ap;
    va_start(ap, fmt);

    // Print error with format
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");

    // Print stack trace from innermost frame to top-level script
    for (uint8_t i = vm->frame_count; i-- > 0;) {
        const VtxCallFrame* frame = &vm->frames[i];
        const size_t idx = frame->ip > 0 ? frame->ip - 1 : 0;
        const size_t line = frame->chunk != NULL ? vtx_chunk_get_line(frame->chunk, idx) : 0;

        const char* fn_name = "script";
        if (frame->closure_id != VTX_INVALID_CLOSURE_ID) {
            const VtxObjClosure* closure = vtx_heap_get_if_const(
                &vm->heap, frame->closure_id, VTX_HEAP_OBJECT_CLOSURE
            );
            if (closure != NULL) {
                const VtxObjFunction* fn = vtx_heap_get_if_const(
                    &vm->heap, closure->function_id, VTX_HEAP_OBJECT_FUNCTION
                );
                if (fn != NULL && fn->name != NULL) {
                    fn_name = fn->name;
                }
            }
        }

        fprintf(stderr, "[line %zu] in %s\n", line, fn_name);
    }

    vtx_vm_reset_stack(vm);
}

/**
 * Reads an 8-bit identifier operand from the instruction stream and resolves its 32-bit name hash/ID.
 *
 * @param frame Active call frame whose instruction pointer is advanced.
 * @return Resolved 32-bit name identifier.
 */
static uint32_t read_name_id(VtxCallFrame* frame) {
    assert(frame != NULL && "Frame pointer is NULL.");

    uint8_t const_idx = vtx_call_frame_read_byte(frame);
    if (const_idx < frame->chunk->string_constants_count) {
        return vtx_hash_string(frame->chunk->string_constants[const_idx]);
    }

    if (const_idx < frame->chunk->constants_count && vtx_value_is_obj(frame->chunk->constants[const_idx])) {
        return (uint32_t)vtx_value_as_obj(frame->chunk->constants[const_idx]);
    }

    return (uint32_t)const_idx;
}

/**
 * Core bytecode execution and instruction dispatch loop.
 *
 * @param vm Pointer to the active virtual machine.
 * @return Execution result of the top-level script or active frame.
 */
static VtxInterpretResult run(VtxVM* vm) {
    assert(vm != NULL && "VM pointer is NULL.");

    while (true) {
        // Check if there are no more frames to process
        if (vm->frame_count == 0) return VTX_INTERPRET_OK;

        // Get the last frame and the associated entry opcode
        VtxCallFrame* frame = &vm->frames[vm->frame_count - 1];
        VtxOpcode opcode = (VtxOpcode)vtx_call_frame_read_byte(frame);

        // Match the opcode and execute the associated operation
        switch (opcode) {
            case VTX_OP_CONSTANT: {
                VtxValue constant = vtx_call_frame_read_constant(frame);
                vtx_vm_push(vm, constant);
                break;
            }
            case VTX_OP_NOP: vtx_vm_push(vm, vtx_value_null()); break;
            case VTX_OP_TRUE: vtx_vm_push(vm, vtx_value_bool(true)); break;
            case VTX_OP_FALSE: vtx_vm_push(vm, vtx_value_bool(false)); break;
            case VTX_OP_POP: { VtxValue pop = vtx_vm_pop(vm); (void)pop; break; }
            case VTX_OP_RETURN: {
                VtxValue result = vtx_vm_pop(vm);
                size_t closing_slots = vm->frames[--vm->frame_count].slots;
            }
        }
    }
}


void vtx_vm_init(VtxVM* vm) {
    assert(vm != NULL && "VM pointer is NULL.");

    // Reset counters
    vm->frame_count = 0;
    vm->stack_count = 0;
    vm->open_upvalues_count = 0;

    // Set initial capacity
    vm->stack = (VtxValue*)malloc(sizeof(VtxValue) * VTX_VM_STACK_INITIAL_CAPACITY);
    assert(vm->stack != NULL && "Out of memory, cannot allocate VM stack.");
    vm->stack_capacity = VTX_VM_STACK_INITIAL_CAPACITY;

    // Set open upvalues to NULL
    vm->open_upvalues = NULL;
    vm->open_upvalues_capacity = 0;

    vtx_heap_init(&vm->heap);

    // Initialise tables
    vtx_table_init(&vm->globals);
    vtx_table_init(&vm->natives);

    // Setup internal flags
    vm->jit_enabled = true;
    vm->dump_asm = false;
    vm->dump_ir = false;
}

void vtx_vm_free(VtxVM* vm) {
    assert(vm != NULL && "VM pointer is NULL.");
    free(vm->stack);
    vtx_heap_free(&vm->heap);
    free(vm->open_upvalues);
    vtx_table_free(&vm->globals);
    vtx_table_free(&vm->natives);
    vm->frame_count = 0;
}

VtxInterpretResult vtx_vm_interpret_from_source(VtxVM* vm, const char* source) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(source != NULL && "Source code string pointer is NULL.");

    // Create and initialise a new chunk
    VtxChunk chunk;
    vtx_chunk_init(&chunk);

    // Create and initialise a new compiler
    VtxCompiler compiler;
    vtx_compiler_init(&compiler, source, VTX_FT_SCRIPT);

    // Compile the source code
    bool compile_ok = vtx_compiler_compile(&compiler, &chunk);
    if (!compile_ok) {
        vtx_chunk_free(&chunk);
        return VTX_INTERPRET_COMPILE_ERROR;
    }

    // Run code interpreter (or JIT)
    VtxInterpretResult result = vtx_vm_interpret(vm, &chunk);
    vtx_chunk_free(&chunk);

    return result;
}

VtxInterpretResult vtx_vm_interpret(VtxVM* vm, const VtxChunk* chunk) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(chunk != NULL && "Chunk pointer is NULL.");

    // Reset stack and push NOP
    vtx_vm_reset_stack(vm);
    vtx_vm_push(vm, vtx_value_null());

    // Initialise a local chunk copy
    VtxChunk local;
    vtx_chunk_init(&local);

    // Copy the VM chunk to this local copy
    vtx_chunk_copy(&local, chunk);

    // Initialise frames
    vtx_call_frame_init(&vm->frames[0], &local, 0);
    vm->frame_count = 1;

    // Load all chunk objects
    load_chunk_objects(vm, &local);

    // Run the interpreter
    VtxInterpretResult result = run(vm);
    
    // Free temporary objects
    vtx_chunk_free(&local);

    return result;
}

void vtx_vm_push(VtxVM* vm, VtxValue value) {
    assert(vm != NULL && "VM pointer is NULL.");

    if (vm->stack_count + 1 > vm->stack_capacity) {
        size_t new_capacity = vm->stack_capacity < 8 ? 8 : vm->stack_capacity * 2;
        VtxValue* new_stack = (VtxValue*)realloc(vm->stack, sizeof(VtxValue) * new_capacity);
        assert(new_stack != NULL && "Out of memory, cannot reallocate VM stack.");
        vm->stack = new_stack;
        vm->stack_capacity = new_capacity;
    }

    vm->stack[vm->stack_count++] = value;
}

VtxValue vtx_vm_pop(VtxVM* vm) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(vm->stack_count > 0 && "VM stack is empty.");

    return vm->stack[--vm->stack_count];
}

VtxValue vtx_vm_peek(const VtxVM* vm, size_t distance) {
    assert(vm != NULL && "VM pointer is NULL.");

    if (distance >= vm->stack_count) return vtx_value_null();
    return vm->stack[vm->stack_count - 1 - distance];
}

void vtx_vm_reset_stack(VtxVM* vm) {
    assert(vm != NULL && "VM pointer is NULL.");
    
    vm->stack_count = 0;
    vm->frame_count = 0;
    vm->open_upvalues_count = 0;
}

VtxValue vtx_vm_allocate_object(VtxVM* vm, const VtxHeapObject* obj) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(obj != NULL && "Object pointer is NULL.");
    
    const size_t obj_id = vtx_heap_allocate(&vm->heap, obj);
    return vtx_value_obj((uint64_t)obj_id);
}

const VtxHeapObject* vtx_vm_get_object_const(const VtxVM* vm, VtxValue value) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(vtx_value_is_obj(value) && "Value is not an object.");
    const uint64_t obj = vtx_value_as_obj(value);
    return vtx_heap_get_const(&vm->heap, (size_t)obj);
}

VtxHeapObject* vtx_vm_get_object(VtxVM* vm, VtxValue value) {
    assert(vm != NULL && "VM pointer is NULL.");
    assert(vtx_value_is_obj(value) && "Value is not an object.");
    const uint64_t obj = vtx_value_as_obj(value);
    return vtx_heap_get(&vm->heap, (size_t)obj);
}

void vtx_vm_define_native(VtxVM* vm, uint32_t name_id, VtxNativeFn native) {
    assert(vm != NULL && "VM pointer is NULL.");
    const bool ok = vtx_table_set_native_by_hash(&vm->natives, name_id, native);
    (void)ok;
}
