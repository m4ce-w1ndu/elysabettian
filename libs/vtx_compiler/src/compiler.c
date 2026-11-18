#include "compiler_internal.h"
#include "vtx/compiler/parser.h"
#include "vtx/compiler/scope.h"
#include "vtx/compiler/token.h"
#include "vtx/core/chunk.h"
#include "vtx/core/opcode.h"

#include <limits.h>
#include <stdio.h>

/**
 * Initializes an VtxCompiler instance with source string and target function kind.
 */
void vtx_compiler_init(VtxCompiler* compiler, const char* source, VtxFunctionType type) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    vtx_parser_init(&compiler->parser, source);
    vtx_compiler_context_init(&compiler->context, type);
    vtx_chunk_init(&compiler->chunk);
}

/**
 * Compiles the entire token stream into an executable bytecode Chunk.
 * On success, transfers ownership of internal chunk buffers to out_chunk.
 */
VTX_NODISCARD bool vtx_compiler_compile(VtxCompiler* compiler, VtxChunk* out_chunk) {
    assert(compiler != NULL && "Compiler pointer is null.");

    vtx_parser_advance(&compiler->parser);

    while (!vtx_parser_match_token(&compiler->parser, VTX_TT_EOF)) {
        vtx_compiler_declaration(compiler);
    }

    vtx_compiler_emit_return(compiler);

    if (compiler->parser.had_error) {
        vtx_chunk_free(&compiler->chunk);
        return false;
    }

    memcpy(out_chunk, &compiler->chunk, sizeof(struct VtxChunk));
    vtx_chunk_init(&compiler->chunk);
    return true;
}

/**
 * Returns the source line number of the previous consumed token.
 */
uint32_t vtx_compiler_current_line(const VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    return compiler->parser.previous.line;
}

/**
 * Writes a single byte into the compiled chunk at the current line.
 */
void vtx_compiler_emit_byte(VtxCompiler* compiler, uint8_t byte) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    vtx_chunk_write(&compiler->chunk, byte, vtx_compiler_current_line(compiler));
}

/**
 * Writes an opcode byte into the compiled chunk at the current line.
 */
void vtx_compiler_emit_opcode(VtxCompiler* compiler, VtxOpcode op) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    vtx_compiler_emit_byte(compiler, (uint8_t)op);
}

/**
 * Emits two consecutive bytes into the compiled chunk.
 */
void vtx_compiler_emit_bytes(VtxCompiler* compiler, uint8_t byte1, uint8_t byte2) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    vtx_compiler_emit_byte(compiler, byte1);
    vtx_compiler_emit_byte(compiler, byte2);
}

/**
 * Emits the default function return sequence (NOP followed by RETURN).
 */
void vtx_compiler_emit_return(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    vtx_compiler_emit_opcode(compiler, VTX_OP_NOP);
    vtx_compiler_emit_opcode(compiler, VTX_OP_RETURN);
}

/**
 * Stores a value into the constant pool and emits VTX_OP_CONSTANT with 8-bit index.
 */
void vtx_compiler_emit_constant(VtxCompiler* compiler, VtxValue value) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    const size_t index = vtx_chunk_add_constant(&compiler->chunk, value);

    if (index > UINT8_MAX) {
        vtx_parser_error(
            &compiler->parser,
            "Constant pool limit exceeded (maximum 256 entries per chunk)."
        );
        return;
    }

    vtx_compiler_emit_opcode(compiler, VTX_OP_CONSTANT);
    vtx_compiler_emit_byte(compiler, (uint8_t)index);
}

/**
 * Emits a forward jump opcode with 16-bit placeholder and returns operand offset.
 */
size_t vtx_compiler_emit_jump(VtxCompiler* compiler, VtxOpcode jump_op) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    vtx_compiler_emit_opcode(compiler, jump_op);
    vtx_compiler_emit_byte(compiler, 0xFF);
    vtx_compiler_emit_byte(compiler, 0xFF);
    return compiler->chunk.code_count - 2;
}

/**
 * Patches a 16-bit forward jump offset with the relative displacement.
 */
void vtx_compiler_patch_jump(VtxCompiler* compiler, size_t offset) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const size_t jump = compiler->chunk.code_count - offset - 2;
    if (jump > UINT16_MAX) {
        vtx_parser_error(
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
void vtx_compiler_emit_loop(VtxCompiler* compiler, size_t loop_start) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const size_t offset = compiler->chunk.code_count - loop_start + 2;
    if (offset > UINT16_MAX) {
        vtx_parser_error(
            &compiler->parser,
            "Backward branch offset out of range: jump distance exceeds 16-bit operand limit (65535 bytes)."
        );
    }

    vtx_compiler_emit_byte(compiler, (uint8_t)((offset >> 8) & 0xFF));
    vtx_compiler_emit_byte(compiler, (uint8_t)(offset & 0xFF));
}
