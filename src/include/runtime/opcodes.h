#ifndef ELY_RT_OPCODE_H
#define ELY_RT_OPCODE_H

// Elysabettian language headers.
#include "common.h"

/**
 * @brief Virtual Machine bytecode opcodes.
*/
enum class Opcode : uint8_t {
    Constant,
    Nop,
    True,
    False,
    Pop,
    GetLocal,
    GetGlobal,
    DefineGlobal,
    SetLocal,
    SetGlobal,
    GetUpvalue,
    SetUpvalue,
    GetProperty,
    SetProperty,
    GetSuper,
    Equal,
    Greater,
    Less,
    Add,
    Subtract,
    Multiply,
    Divide,
    Not,
    Negate,
    Print,
    Jump,
    JumpIfFalse,
    Loop,
    Call,
    Invoke,
    SuperInvoke,
    Closure,
    CloseUpvalue,
    Return,
    Class,
    Inherit,
    Method,
    BwAnd,
    BwOr,
    BwXor,
    BwNot,
    ArrBuild,
    ArrIndex,
    ArrStore,
    ShiftLeft,
    ShiftRight
};


#endif /* opcode_hpp */
