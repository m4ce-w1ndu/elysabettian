//
//  chunk.h
//  Elysabettian
//
//  Created by Simone Rolando on 06/07/21.
//

#ifndef chunk_h
#define chunk_h

#include "common.h"
#include "value.h"

typedef enum OpCode {
    OP_CONSTANT, OP_NULL, OP_FALSE, OP_TRUE,
    OP_POP, OP_GET_LOCAL, OP_SET_LOCAL,
    OP_GET_GLOBAL, OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL, OP_GET_UPVALUE, OP_SET_UPVALUE,
    OP_GET_PROPERTY, OP_SET_PROPERTY, OP_GET_SUPER,
    OP_EQUAL, OP_GREATER, OP_LESS, OP_ADD, OP_SUB,
    OP_MUL, OP_DIV, OP_NOT, OP_NEGATE, OP_PRINT,
    OP_MOD, OP_BITWISE_AND, OP_BITWISE_OR, OP_BITWISE_XOR,
    OP_BITWISE_NOT, OP_JUMP, OP_JUMP_IF_FALSE, OP_LOOP,
    OP_CALL, OP_INVOKE, OP_SUPER_INVOKE, OP_CLOSURE,
    OP_CLOSE_UPVALUE, OP_RETURN, OP_CLASS, OP_INHERIT,
    OP_METHOD
} OpCode;

typedef struct Chunk {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
void write_chunk(Chunk* chunk, uint8_t byte, int line);
int add_constant(Chunk* chunk, Value value);

#endif /* chunk_h */
