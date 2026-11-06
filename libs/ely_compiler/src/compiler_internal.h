#ifndef ELY_COMPILER_INTERNAL_H
#define ELY_COMPILER_INTERNAL_H

#include <ely/compiler/compiler.h>
#include <ely/compiler/parser.h>
#include <ely/compiler/scope.h>
#include <ely/compiler/token.h>
#include <ely/core/chunk.h>
#include <ely/core/opcode.h>
#include <ely/core/value.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Bytecode emission (compiler.c)
ELY_NODISCARD uint32_t ely_compiler_current_line(const ElyCompiler* compiler);
void ely_compiler_emit_byte(ElyCompiler* compiler, uint8_t byte);
void ely_compiler_emit_opcode(ElyCompiler* compiler, ElyOpcode op);
void ely_compiler_emit_bytes(ElyCompiler* compiler, uint8_t byte1, uint8_t byte2);
void ely_compiler_emit_return(ElyCompiler* compiler);
void ely_compiler_emit_constant(ElyCompiler* compiler, ElyValue value);
ELY_NODISCARD size_t ely_compiler_emit_jump(ElyCompiler* compiler, ElyOpcode jump_op);
void ely_compiler_patch_jump(ElyCompiler* compiler, size_t offset);
void ely_compiler_emit_loop(ElyCompiler* compiler, size_t loop_start);

// Scope and variable bindings (compiler_scope.c)
void ely_compiler_begin_scope(ElyCompiler* compiler);
void ely_compiler_end_scope(ElyCompiler* compiler);
void ely_compiler_declare_variable(ElyCompiler* compiler);
void ely_compiler_mark_initialized(ElyCompiler* compiler);
ELY_NODISCARD bool ely_compiler_resolve_local(ElyCompiler* compiler, const ElyToken* name, uint8_t* out_slot);
ELY_NODISCARD uint8_t ely_compiler_identifier_constant(ElyCompiler* compiler, const ElyToken* name);

// Expression parsing and Pratt rules (compiler_expr.c)
void ely_compiler_expression(ElyCompiler* compiler);
void ely_compiler_parse_precedence(ElyCompiler* compiler, ElyPrecedence precedence);
ELY_NODISCARD ElyPrecedence ely_compiler_get_precedence(ElyTokenType token_type);

// Statements and declarations (compiler_stmt.c)
void ely_compiler_declaration(ElyCompiler* compiler);
void ely_compiler_statement(ElyCompiler* compiler);
void ely_compiler_block(ElyCompiler* compiler);

#endif
