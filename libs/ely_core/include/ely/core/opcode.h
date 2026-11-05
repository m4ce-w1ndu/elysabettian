#ifndef ELY_CORE_OPCODE_H
#define ELY_CORE_OPCODE_H

/**
 * @brief Bytecode Instruction Set Architecture (ISA) for the Elysabettian VM.
 *
 * Each opcode is an 8-bit unsigned integer (uint8_t) executed by the stack-based
 * interpreter or lowered to native machine code by the JIT compiler.
 */

#include "value.h"

typedef enum ElyOpcode {
    // Stack and constants
    ELY_OP_CONSTANT = 0,
    ELY_OP_NOP,
    ELY_OP_TRUE,
    ELY_OP_FALSE,
    ELY_OP_POP,

    // Variables and bindings
    ELY_OP_GETLOCAL,
    ELY_OP_GETGLOBAL,
    ELY_OP_DEFINEGLOBAL,
    ELY_OP_SETLOCAL,
    ELY_OP_SETGLOBAL,
    ELY_OP_GETUPVALUE,
    ELY_OP_SETUPVALUE,
    ELY_OP_GETPROPERTY,
    ELY_OP_SETPROPERTY,
    ELY_OP_GETSUPER,

    // Comparisons
    ELY_OP_EQUAL,
    ELY_OP_GREATER,
    ELY_OP_LESS,

    // Arithmetic and logical operations
    ELY_OP_ADD,
    ELY_OP_SUBTRACT,
    ELY_OP_MULTIPLY,
    ELY_OP_DIVIDE,
    ELY_OP_NOT,
    ELY_OP_NEGATE,

    // Control flow and output
    ELY_OP_PRINT,
    ELY_OP_JUMP,
    ELY_OP_JUMPZERO,
    ELY_OP_LOOP,

    // Functions and closures
    ELY_OP_CALL,
    ELY_OP_INVOKE,
    ELY_OP_SUPERINVOKE,
    ELY_OP_CLOSURE,
    ELY_OP_CLOSEUPVALUE,
    ELY_OP_RETURN,

    // Classes and inheritance
    ELY_OP_CLASS,
    ELY_OP_INHERIT,
    ELY_OP_METHOD,

    // Bitwise operations
    ELY_OP_BWAND,
    ELY_OP_BWOR,
    ELY_OP_BWXOR,
    ELY_OP_BWNOT,
    ELY_OP_SHIFTLEFT,
    ELY_OP_SHIFTRIGHT,

    // Arrays
    ELY_OP_ARRAYBUILD,
    ELY_OP_ARRAYINDEX,
    ELY_OP_ARRAYSTORE,

    // Modules
    ELY_OP_IMPORT
} ElyOpcode;

/**
 * @brief Returns a human-readable string representation of an opcode.
 *
 * @param op The opcode to convert.
 * @return String representation of the opcode.
 */
ELY_NODISCARD static inline const char *ely_opcode_name(ElyOpcode op) {
    switch (op) {
        case ELY_OP_CONSTANT:     return "Constant";
        case ELY_OP_NOP:          return "Nop";
        case ELY_OP_TRUE:         return "True";
        case ELY_OP_FALSE:        return "False";
        case ELY_OP_POP:          return "Pop";
        case ELY_OP_GETLOCAL:     return "GetLocal";
        case ELY_OP_GETGLOBAL:    return "GetGlobal";
        case ELY_OP_DEFINEGLOBAL: return "DefineGlobal";
        case ELY_OP_SETLOCAL:     return "SetLocal";
        case ELY_OP_SETGLOBAL:    return "SetGlobal";
        case ELY_OP_GETUPVALUE:   return "GetUpvalue";
        case ELY_OP_SETUPVALUE:   return "SetUpvalue";
        case ELY_OP_GETPROPERTY:  return "GetProperty";
        case ELY_OP_SETPROPERTY:  return "SetProperty";
        case ELY_OP_GETSUPER:     return "GetSuper";
        case ELY_OP_EQUAL:        return "Equal";
        case ELY_OP_GREATER:      return "Greater";
        case ELY_OP_LESS:         return "Less";
        case ELY_OP_ADD:          return "Add";
        case ELY_OP_SUBTRACT:     return "Subtract";
        case ELY_OP_MULTIPLY:     return "Multiply";
        case ELY_OP_DIVIDE:       return "Divide";
        case ELY_OP_NOT:          return "Not";
        case ELY_OP_NEGATE:       return "Negate";
        case ELY_OP_PRINT:        return "Print";
        case ELY_OP_JUMP:         return "Jump";
        case ELY_OP_JUMPZERO:     return "JumpZero";
        case ELY_OP_LOOP:         return "Loop";
        case ELY_OP_CALL:         return "Call";
        case ELY_OP_INVOKE:       return "Invoke";
        case ELY_OP_SUPERINVOKE:  return "SuperInvoke";
        case ELY_OP_CLOSURE:      return "Closure";
        case ELY_OP_CLOSEUPVALUE: return "CloseUpvalue";
        case ELY_OP_RETURN:       return "Return";
        case ELY_OP_CLASS:        return "Class";
        case ELY_OP_INHERIT:      return "Inherit";
        case ELY_OP_METHOD:       return "Method";
        case ELY_OP_BWAND:        return "BwAnd";
        case ELY_OP_BWOR:         return "BwOr";
        case ELY_OP_BWXOR:        return "BwXor";
        case ELY_OP_BWNOT:        return "BwNot";
        case ELY_OP_SHIFTLEFT:    return "ShiftLeft";
        case ELY_OP_SHIFTRIGHT:   return "ShiftRight";
        case ELY_OP_ARRAYBUILD:   return "ArrayBuild";
        case ELY_OP_ARRAYINDEX:   return "ArrayIndex";
        case ELY_OP_ARRAYSTORE:   return "ArrayStore";
        case ELY_OP_IMPORT:       return "Import";
    }
    return "Unknown";
}

#endif
