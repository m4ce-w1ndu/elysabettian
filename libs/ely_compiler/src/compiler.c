#include "compiler_internal.h"
#include "ely/compiler/parser.h"
#include "ely/compiler/scope.h"
#include "ely/compiler/token.h"
#include "ely/core/chunk.h"
#include "ely/core/opcode.h"

#include <limits.h>
#include <stdio.h>

/**
 * Initializes an ElyCompiler instance with source string and target function kind.
 */
void ely_compiler_init(ElyCompiler* compiler, const char* source, ElyFunctionType type) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    ely_parser_init(&compiler->parser, source);
    ely_compiler_context_init(&compiler->context, type);
    ely_chunk_init(&compiler->chunk);
}

/**
 * Compiles the entire token stream into an executable bytecode Chunk.
 * On success, transfers ownership of internal chunk buffers to out_chunk.
 */
ELY_NODISCARD bool ely_compiler_compile(ElyCompiler* compiler, ElyChunk* out_chunk) {
    assert(compiler != NULL && "Compiler pointer is null.");

    ely_parser_advance(&compiler->parser);

    while (!ely_parser_match_token(&compiler->parser, ELY_TT_EOF)) {
        ely_compiler_declaration(compiler);
    }

    ely_compiler_emit_return(compiler);

    if (compiler->parser.had_error) {
        ely_chunk_free(&compiler->chunk);
        return false;
    }

    memcpy(out_chunk, &compiler->chunk, sizeof(struct ElyChunk));
    ely_chunk_init(&compiler->chunk);
    return true;
}

/**
 * Returns the source line number of the previous consumed token.
 */
uint32_t ely_compiler_current_line(const ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    return compiler->parser.previous.line;
}

/**
 * Writes a single byte into the compiled chunk at the current line.
 */
void ely_compiler_emit_byte(ElyCompiler* compiler, uint8_t byte) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    ely_chunk_write(&compiler->chunk, byte, ely_compiler_current_line(compiler));
}

/**
 * Writes an opcode byte into the compiled chunk at the current line.
 */
void ely_compiler_emit_opcode(ElyCompiler* compiler, ElyOpcode op) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    ely_compiler_emit_byte(compiler, (uint8_t)op);
}

/**
 * Emits two consecutive bytes into the compiled chunk.
 */
void ely_compiler_emit_bytes(ElyCompiler* compiler, uint8_t byte1, uint8_t byte2) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    ely_compiler_emit_byte(compiler, byte1);
    ely_compiler_emit_byte(compiler, byte2);
}

/**
 * Emits the default function return sequence (NOP followed by RETURN).
 */
void ely_compiler_emit_return(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    ely_compiler_emit_opcode(compiler, ELY_OP_NOP);
    ely_compiler_emit_opcode(compiler, ELY_OP_RETURN);
}

/**
 * Stores a value into the constant pool and emits ELY_OP_CONSTANT with 8-bit index.
 */
void ely_compiler_emit_constant(ElyCompiler* compiler, ElyValue value) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    const size_t index = ely_chunk_add_constant(&compiler->chunk, value);

    if (index > UINT8_MAX) {
        ely_parser_error(
            &compiler->parser,
            "Constant pool limit exceeded (maximum 256 entries per chunk)."
        );
        return;
    }

    ely_compiler_emit_opcode(compiler, ELY_OP_CONSTANT);
    ely_compiler_emit_byte(compiler, (uint8_t)index);
}

/**
 * Emits a forward jump opcode with 16-bit placeholder and returns operand offset.
 */
size_t ely_compiler_emit_jump(ElyCompiler* compiler, ElyOpcode jump_op) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    ely_compiler_emit_opcode(compiler, jump_op);
    ely_compiler_emit_byte(compiler, 0xFF);
    ely_compiler_emit_byte(compiler, 0xFF);
    return compiler->chunk.code_count - 2;
}

/**
 * Patches a 16-bit forward jump offset with the relative displacement.
 */
void ely_compiler_patch_jump(ElyCompiler* compiler, size_t offset) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const size_t jump = compiler->chunk.code_count - offset - 2;
    if (jump > UINT16_MAX) {
        ely_parser_error(
            &compiler->parser,
            "Branch offset out of range: jump distance exceeds 16-bit operand limit (65535 bytes)."
        );
    }

    compiler->chunk.code[offset] = (uint8_t)((jump >> 8) & 0xFF);
    compiler->chunk.code[offset + 1] = (uint8_t)(jump & 0xFF);
}

/**
 * Emits a backward branch opcode (LOOP) targeting a loop start offset.
 */
void ely_compiler_emit_loop(ElyCompiler* compiler, size_t loop_start) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const size_t offset = compiler->chunk.code_count - loop_start + 2;
    if (offset > UINT16_MAX) {
        ely_parser_error(
            &compiler->parser,
            "Backward branch offset out of range: jump distance exceeds 16-bit operand limit (65535 bytes)."
        );
    }

    ely_compiler_emit_byte(compiler, (uint8_t)((offset >> 8) & 0xFF));
    ely_compiler_emit_byte(compiler, (uint8_t)(offset & 0xFF));
}
