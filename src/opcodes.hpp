#ifndef OPCODE_HPP
#define OPCODE_HPP

#include "common.hpp"

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
};


#endif /* opcode_hpp */
