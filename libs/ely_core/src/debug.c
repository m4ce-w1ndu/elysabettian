#include "ely/core/chunk.h"
#include "ely/core/opcode.h"
#include "ely/core/value.h"
#include <ely/core/debug.h>

#include <stddef.h>
#include <stdio.h>

/**
 * Prints a value by determining its type.
 */
static void print_value(ElyValue value) {
    if (ely_value_is_number(value)) {
        printf("%g", ely_value_as_number(value));
    } else if (ely_value_is_null(value)) {
        printf("null");
    } else if (ely_value_is_bool(value)) {
        printf("%s", ely_value_as_bool(value) ? "true" : "false");
    } else if (ely_value_is_obj(value)) {
        printf("<object %lx>", ely_value_as_obj(value));
    }
}

/**
 * Prints a zero-operand instruction and returns the offset of the next instruction.
 */
ELY_NODISCARD static size_t simple_instruction(const char* name, size_t offset) {
    printf("%s", name);
    return offset + 1;
}

/**
 * Prints an instruction with a single 1-byte operand (such as stack slots or argument counts).
 */
ELY_NODISCARD static size_t byte_instruction(const char* name, const ElyChunk* chunk, size_t offset) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    uint8_t slot = chunk->code[offset + 1];
    printf("%s %4d\n", name, slot);
    return offset + 2;
}

/**
 * Prints an instruction that references a constant from the constant pool.
 */
ELY_NODISCARD static size_t constant_instruction(const char* name, const ElyChunk* chunk, size_t offset) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    const uint8_t const_idx = chunk->code[offset + 1];
    const ElyValue value = chunk->constants[const_idx];

    printf("%-16s %4d '", name, const_idx);
    print_value(value);
    printf("'\n");

    return offset + 2;
}

ELY_NODISCARD static size_t jump_instruction(const char* name, int32_t sign, const ElyChunk* chunk, size_t offset) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    const uint16_t hi = (uint16_t)chunk->code[offset + 1];
    const uint16_t lo = (uint16_t)chunk->code[offset + 2];
    const uint16_t jump = (uint16_t)(hi << 8 | lo);
    const size_t target = (size_t)((ptrdiff_t)(offset + 3) + (ptrdiff_t)sign * jump);

    printf("%-16s %4zu -> %zu\n", name, offset, target);

    return offset + 3;
}

void ely_debug_disassemble_chunk(const ElyChunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    size_t offset = 0;

    while (offset < chunk->code_count) offset = ely_debug_disassemble_instruction(chunk, offset);
}

ELY_NODISCARD size_t ely_debug_disassemble_instruction(const ElyChunk* chunk, size_t offset) {
    assert(chunk != NULL && "Chunk pointer is NULL.");

    printf("%4zu ", offset);

    const size_t line = ely_chunk_get_line(chunk, offset);
    if (offset > 0 && line == ely_chunk_get_line(chunk, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4zu ", line);
    }

    if (offset >= chunk->code_count) {
        printf("Unexpected end of chunk at offset %zu\n", offset);
        return offset + 1;
    }

    const uint8_t instruction = chunk->code[offset];
    if (instruction > (uint8_t)(ELY_OP_IMPORT)) {
        printf("Unknown opcode: %4x\n", instruction);
        return offset + 1;
    }

    const ElyOpcode opcode = (ElyOpcode)instruction;

    switch (opcode) {
        case ELY_OP_CONSTANT: return constant_instruction("const", chunk, offset);
        case ELY_OP_NOP: return simple_instruction("nop", offset);
        case ELY_OP_TRUE: return simple_instruction("true", offset);
        case ELY_OP_FALSE: return simple_instruction("false", offset);
        case ELY_OP_POP: return simple_instruction("pop", offset);
        case ELY_OP_GETLOCAL: return byte_instruction("getlcl", chunk, offset);
        case ELY_OP_GETGLOBAL: return constant_instruction("getglb", chunk, offset);
        case ELY_OP_DEFINEGLOBAL: return constant_instruction("defglb", chunk, offset);
        case ELY_OP_SETLOCAL: return byte_instruction("setlcl", chunk, offset);
        case ELY_OP_SETGLOBAL: return constant_instruction("setglb", chunk, offset);
        case ELY_OP_GETUPVALUE: return byte_instruction("getupv", chunk, offset);
        case ELY_OP_SETUPVALUE: return byte_instruction("setupv", chunk, offset);
        case ELY_OP_GETPROPERTY: return constant_instruction("getprp", chunk, offset);
        case ELY_OP_SETPROPERTY: return constant_instruction("setprp", chunk, offset);
        case ELY_OP_GETSUPER: return constant_instruction("getsup", chunk, offset);
        case ELY_OP_EQUAL: return simple_instruction("eq", offset);
        case ELY_OP_LESS: return simple_instruction("lt", offset);
        case ELY_OP_GREATER: return simple_instruction("gt", offset);
        case ELY_OP_ADD: return simple_instruction("add", offset);
        case ELY_OP_SUBTRACT: return simple_instruction("sub", offset);
        case ELY_OP_MULTIPLY: return simple_instruction("mul", offset);
        case ELY_OP_DIVIDE: return simple_instruction("div", offset);
        case ELY_OP_NOT: return simple_instruction("not", offset);
        case ELY_OP_NEGATE: return simple_instruction("neg", offset);
        case ELY_OP_PRINT: return simple_instruction("prn", offset);
        case ELY_OP_JUMP: return jump_instruction("jmp", 1, chunk, offset);
        case ELY_OP_JUMPZERO: return jump_instruction("jz", 1, chunk, offset);
        case ELY_OP_LOOP: return jump_instruction("iter", -1, chunk, offset);
        case ELY_OP_CALL: return byte_instruction("call", chunk, offset);
        case ELY_OP_INVOKE: {
            const uint8_t name_idx = chunk->code[offset + 1];
            const uint8_t argc = chunk->code[offset + 2];
            const ElyValue value = chunk->constants[name_idx];

            printf("%-16s (%d args) %4d '", "invk", argc, name_idx);
            print_value(value);
            printf("'\n");

            return offset + 3;
        }
        case ELY_OP_SUPERINVOKE: {
            const uint8_t name_idx = chunk->code[offset + 1];
            const uint8_t argc = chunk->code[offset + 2];
            const ElyValue value = chunk->constants[name_idx];

            printf("%-16s (%d args) %4d '", "sinvk", argc, name_idx);
            print_value(value);
            printf("'\n");

            return offset + 3;
        }
        case ELY_OP_CLOSURE: {
            size_t curr = offset + 1;
            const uint8_t const_idx = chunk->code[curr];
            const ElyValue value = chunk->constants[const_idx];

            printf("%-16s %4d '", "closure", const_idx);
            print_value(value);
            printf("'\n");
            curr++;

            return curr;
        }
        case ELY_OP_CLOSEUPVALUE: return simple_instruction("clsupv", offset);
        case ELY_OP_RETURN: return simple_instruction("ret", offset);
        case ELY_OP_CLASS: return constant_instruction("class", chunk, offset);
        case ELY_OP_INHERIT: return simple_instruction("inhrt", offset);
        case ELY_OP_METHOD: return constant_instruction("mthd", chunk, offset);
        case ELY_OP_BWAND: return simple_instruction("and", offset);
        case ELY_OP_BWOR: return simple_instruction("or", offset);
        case ELY_OP_BWXOR: return simple_instruction("xor", offset);
        case ELY_OP_BWNOT: return simple_instruction("not", offset);
        case ELY_OP_SHIFTLEFT: return simple_instruction("shl", offset);
        case ELY_OP_SHIFTRIGHT: return simple_instruction("shr", offset);
        case ELY_OP_ARRAYBUILD: return byte_instruction("arrbld", chunk, offset);
        case ELY_OP_ARRAYSTORE: return simple_instruction("arrstr", offset);
        case ELY_OP_ARRAYINDEX: return simple_instruction("arridx", offset);
        case ELY_OP_IMPORT: return byte_instruction("import", chunk, offset);
    }

    return offset + 1;
}
