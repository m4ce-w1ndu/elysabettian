#ifndef VTX_COMPILER_INTERNAL_H
#define VTX_COMPILER_INTERNAL_H

#include <vtx/compiler/compiler.h>
#include <vtx/compiler/parser.h>
#include <vtx/compiler/scope.h>
#include <vtx/compiler/token.h>
#include <vtx/core/chunk.h>
#include <vtx/core/opcode.h>
#include <vtx/core/value.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Bytecode emission (compiler.c)
VTX_NODISCARD uint32_t vtx_compiler_current_line(const VtxCompiler* compiler);
void vtx_compiler_emit_byte(VtxCompiler* compiler, uint8_t byte);
void vtx_compiler_emit_opcode(VtxCompiler* compiler, VtxOpcode op);
void vtx_compiler_emit_bytes(VtxCompiler* compiler, uint8_t byte1, uint8_t byte2);
void vtx_compiler_emit_return(VtxCompiler* compiler);
void vtx_compiler_emit_constant(VtxCompiler* compiler, VtxValue value);
VTX_NODISCARD size_t vtx_compiler_emit_jump(VtxCompiler* compiler, VtxOpcode jump_op);
void vtx_compiler_patch_jump(VtxCompiler* compiler, size_t offset);
void vtx_compiler_emit_loop(VtxCompiler* compiler, size_t loop_start);

// Scope and variable bindings (compiler_scope.c)
void vtx_compiler_begin_scope(VtxCompiler* compiler);
void vtx_compiler_end_scope(VtxCompiler* compiler);
void vtx_compiler_declare_variable(VtxCompiler* compiler);
void vtx_compiler_mark_initialized(VtxCompiler* compiler);
VTX_NODISCARD bool vtx_compiler_resolve_local(VtxCompiler* compiler, const VtxToken* name, uint8_t* out_slot);
VTX_NODISCARD uint8_t vtx_compiler_identifier_constant(VtxCompiler* compiler, const VtxToken* name);

// Expression parsing and Pratt rules (compiler_expr.c)
void vtx_compiler_expression(VtxCompiler* compiler);
void vtx_compiler_parse_precedence(VtxCompiler* compiler, VtxPrecedence precedence);
VTX_NODISCARD VtxPrecedence vtx_compiler_get_precedence(VtxTokenType token_type);

// Statements and declarations (compiler_stmt.c)
void vtx_compiler_declaration(VtxCompiler* compiler);
void vtx_compiler_statement(VtxCompiler* compiler);
void vtx_compiler_block(VtxCompiler* compiler);

#endif
