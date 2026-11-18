#include "vtx/core/chunk.h"
#include "vtx/core/opcode.h"
#include "vtx/core/value.h"
#include <vtx/core/debug.h>

#include <stddef.h>
#include <stdio.h>

/**
 * Prints a value by determining its type.
 */
static void print_value(VtxValue value) {
    if (vtx_value_is_number(value)) {
        printf("%g", vtx_value_as_number(value));
    } else if (vtx_value_is_null(value)) {
        printf("null");
    } else if (vtx_value_is_bool(value)) {
        printf("%s", vtx_value_as_bool(value) ? "true" : "false");
    } else if (vtx_value_is_obj(value)) {
        printf("<object %lx>", vtx_value_as_obj(value));
    }
}

/**
 * Prints a zero-operand instruction and returns the offset of the next instruction.
 */
VTX_NODISCARD static size_t simple_instruction(const char* name, size_t offset) {
    printf("%s", name);
    return offset + 1;
}

/**
 * Prints an instruction with a single 1-byte operand (such as stack slots or argument counts).
 */
VTX_NODISCARD static size_t byte_instruction(const char* name, const VtxChunk* chunk, size_t offset) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    uint8_t slot = chunk->code[offset + 1];
    printf("%s %4d\n", name, slot);
    return offset + 2;
}

/**
 * Prints an instruction that references a constant from the constant pool.
 */
VTX_NODISCARD static size_t constant_instruction(const char* name, const VtxChunk* chunk, size_t offset) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    const uint8_t const_idx = chunk->code[offset + 1];
    const VtxValue value = chunk->constants[const_idx];

    printf("%-16s %4d '", name, const_idx);
    print_value(value);
    printf("'\n");

    return offset + 2;
}

VTX_NODISCARD static size_t jump_instruction(const char* name, int32_t sign, const VtxChunk* chunk, size_t offset) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    const uint16_t hi = (uint16_t)chunk->code[offset + 1];
    const uint16_t lo = (uint16_t)chunk->code[offset + 2];
    const uint16_t jump = (uint16_t)(hi << 8 | lo);
    const size_t target = (size_t)((ptrdiff_t)(offset + 3) + (ptrdiff_t)sign * jump);

    printf("%-16s %4zu -> %zu\n", name, offset, target);

    return offset + 3;
}

void vtx_debug_disassemble_chunk(const VtxChunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    size_t offset = 0;

    while (offset < chunk->code_count) offset = vtx_debug_disassemble_instruction(chunk, offset);
}

VTX_NODISCARD size_t vtx_debug_disassemble_instruction(const VtxChunk* chunk, size_t offset) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    printf("%4zu ", offset);

    const size_t line = vtx_chunk_get_line(chunk, offset);
    if (offset > 0 && line == vtx_chunk_get_line(chunk, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4zu ", line);
    }

    if (offset >= chunk->code_count) {
        printf("Unexpected end of chunk at offset %zu\n", offset);
        return offset + 1;
    }

    const uint8_t instruction = chunk->code[offset];
    if (instruction > (uint8_t)(VTX_OP_IMPORT)) {
        printf("Unknown opcode: %4x\n", instruction);
        return offset + 1;
    }

    const VtxOpcode opcode = (VtxOpcode)instruction;

    switch (opcode) {
        case VTX_OP_CONSTANT: return constant_instruction("const", chunk, offset);
        case VTX_OP_NOP: return simple_instruction("nop", offset);
        case VTX_OP_TRUE: return simple_instruction("true", offset);
        case VTX_OP_FALSE: return simple_instruction("false", offset);
        case VTX_OP_POP: return simple_instruction("pop", offset);
        case VTX_OP_GETLOCAL: return byte_instruction("getlcl", chunk, offset);
        case VTX_OP_GETGLOBAL: return constant_instruction("getglb", chunk, offset);
        case VTX_OP_DEFINEGLOBAL: return constant_instruction("defglb", chunk, offset);
        case VTX_OP_SETLOCAL: return byte_instruction("setlcl", chunk, offset);
        case VTX_OP_SETGLOBAL: return constant_instruction("setglb", chunk, offset);
        case VTX_OP_GETUPVALUE: return byte_instruction("getupv", chunk, offset);
        case VTX_OP_SETUPVALUE: return byte_instruction("setupv", chunk, offset);
        case VTX_OP_GETPROPERTY: return constant_instruction("getprp", chunk, offset);
        case VTX_OP_SETPROPERTY: return constant_instruction("setprp", chunk, offset);
        case VTX_OP_GETSUPER: return constant_instruction("getsup", chunk, offset);
        case VTX_OP_EQUAL: return simple_instruction("eq", offset);
        case VTX_OP_LESS: return simple_instruction("lt", offset);
        case VTX_OP_GREATER: return simple_instruction("gt", offset);
        case VTX_OP_ADD: return simple_instruction("add", offset);
        case VTX_OP_SUBTRACT: return simple_instruction("sub", offset);
        case VTX_OP_MULTIPLY: return simple_instruction("mul", offset);
        case VTX_OP_DIVIDE: return simple_instruction("div", offset);
        case VTX_OP_NOT: return simple_instruction("not", offset);
        case VTX_OP_NEGATE: return simple_instruction("neg", offset);
        case VTX_OP_PRINT: return simple_instruction("prn", offset);
        case VTX_OP_JUMP: return jump_instruction("jmp", 1, chunk, offset);
        case VTX_OP_JUMPZERO: return jump_instruction("jz", 1, chunk, offset);
        case VTX_OP_LOOP: return jump_instruction("iter", -1, chunk, offset);
        case VTX_OP_CALL: return byte_instruction("call", chunk, offset);
        case VTX_OP_INVOKE: {
            const uint8_t name_idx = chunk->code[offset + 1];
            const uint8_t argc = chunk->code[offset + 2];
            const VtxValue value = chunk->constants[name_idx];

            printf("%-16s (%d args) %4d '", "invk", argc, name_idx);
            print_value(value);
            printf("'\n");

            return offset + 3;
        }
        case VTX_OP_SUPERINVOKE: {
            const uint8_t name_idx = chunk->code[offset + 1];
            const uint8_t argc = chunk->code[offset + 2];
            const VtxValue value = chunk->constants[name_idx];

            printf("%-16s (%d args) %4d '", "sinvk", argc, name_idx);
            print_value(value);
            printf("'\n");

            return offset + 3;
        }
        case VTX_OP_CLOSURE: {
            size_t curr = offset + 1;
            const uint8_t const_idx = chunk->code[curr];
            const VtxValue value = chunk->constants[const_idx];

            printf("%-16s %4d '", "closure", const_idx);
            print_value(value);
            printf("'\n");
            curr++;

            return curr;
        }
        case VTX_OP_CLOSEUPVALUE: return simple_instruction("clsupv", offset);
        case VTX_OP_RETURN: return simple_instruction("ret", offset);
        case VTX_OP_CLASS: return constant_instruction("class", chunk, offset);
        case VTX_OP_INHERIT: return simple_instruction("inhrt", offset);
        case VTX_OP_METHOD: return constant_instruction("mthd", chunk, offset);
        case VTX_OP_BWAND: return simple_instruction("and", offset);
        case VTX_OP_BWOR: return simple_instruction("or", offset);
        case VTX_OP_BWXOR: return simple_instruction("xor", offset);
        case VTX_OP_BWNOT: return simple_instruction("not", offset);
        case VTX_OP_SHIFTLEFT: return simple_instruction("shl", offset);
        case VTX_OP_SHIFTRIGHT: return simple_instruction("shr", offset);
        case VTX_OP_ARRAYBUILD: return byte_instruction("arrbld", chunk, offset);
        case VTX_OP_ARRAYSTORE: return simple_instruction("arrstr", offset);
        case VTX_OP_ARRAYINDEX: return simple_instruction("arridx", offset);
        case VTX_OP_IMPORT: return byte_instruction("import", chunk, offset);
    }

    return offset + 1;
}
