#ifndef OPCODE_HPP
#define OPCODE_HPP

#include "common.hpp"

enum class opcode_t : uint8_t {
    CONSTANT,
    NULLOP,
    TRUE,
    FALSE,
    POP,
    GET_LOCAL,
    GET_GLOBAL,
    DEFINE_GLOBAL,
    SET_LOCAL,
    SET_GLOBAL,
    GET_UPVALUE,
    SET_UPVALUE,
    GET_PROPERTY,
    SET_PROPERTY,
    GET_SUPER,
    EQUAL,
    GREATER,
    LESS,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    NOT,
    NEGATE,
    PRINT,
    JUMP,
    JUMP_IF_FALSE,
    LOOP,
    CALL,
    INVOKE,
    SUPER_INVOKE,
    CLOSURE,
    CLOSE_UPVALUE,
    RETURN,
    CLASS,
    INHERIT,
    METHOD,
    BW_AND,
    BW_OR,
    BW_XOR,
    BW_NOT,
    ARR_BUILD,
    ARR_INDEX,
    ARR_STORE,
};


#endif /* opcode_hpp */
