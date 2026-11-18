#ifndef VTX_CORE_OPCODE_H
#define VTX_CORE_OPCODE_H

/**
 * @brief Bytecode Instruction Set Architecture (ISA) for the Elysabettian VM.
 *
 * Each opcode is an 8-bit unsigned integer (uint8_t) executed by the stack-based
 * interpreter or lowered to native machine code by the JIT compiler.
 */

#include "value.h"

typedef enum VtxOpcode {
    // Stack and constants
    VTX_OP_CONSTANT = 0,
    VTX_OP_NOP,
    VTX_OP_TRUE,
    VTX_OP_FALSE,
    VTX_OP_POP,

    // Variables and bindings
    VTX_OP_GETLOCAL,
    VTX_OP_GETGLOBAL,
    VTX_OP_DEFINEGLOBAL,
    VTX_OP_SETLOCAL,
    VTX_OP_SETGLOBAL,
    VTX_OP_GETUPVALUE,
    VTX_OP_SETUPVALUE,
    VTX_OP_GETPROPERTY,
    VTX_OP_SETPROPERTY,
    VTX_OP_GETSUPER,

    // Comparisons
    VTX_OP_EQUAL,
    VTX_OP_GREATER,
    VTX_OP_LESS,

    // Arithmetic and logical operations
    VTX_OP_ADD,
    VTX_OP_SUBTRACT,
    VTX_OP_MULTIPLY,
    VTX_OP_DIVIDE,
    VTX_OP_NOT,
    VTX_OP_NEGATE,

    // Control flow and output
    VTX_OP_PRINT,
    VTX_OP_JUMP,
    VTX_OP_JUMPZERO,
    VTX_OP_LOOP,

    // Functions and closures
    VTX_OP_CALL,
    VTX_OP_INVOKE,
    VTX_OP_SUPERINVOKE,
    VTX_OP_CLOSURE,
    VTX_OP_CLOSEUPVALUE,
    VTX_OP_RETURN,

    // Classes and inheritance
    VTX_OP_CLASS,
    VTX_OP_INHERIT,
    VTX_OP_METHOD,

    // Bitwise operations
    VTX_OP_BWAND,
    VTX_OP_BWOR,
    VTX_OP_BWXOR,
    VTX_OP_BWNOT,
    VTX_OP_SHIFTLEFT,
    VTX_OP_SHIFTRIGHT,

    // Arrays
    VTX_OP_ARRAYBUILD,
    VTX_OP_ARRAYINDEX,
    VTX_OP_ARRAYSTORE,

    // Modules
    VTX_OP_IMPORT
} VtxOpcode;

/**
 * @brief Returns a human-readable string representation of an opcode.
 *
 * @param op The opcode to convert.
 * @return String representation of the opcode.
 */
VTX_NODISCARD static inline const char *vtx_opcode_name(VtxOpcode op) {
    switch (op) {
        case VTX_OP_CONSTANT:     return "Constant";
        case VTX_OP_NOP:          return "Nop";
        case VTX_OP_TRUE:         return "True";
        case VTX_OP_FALSE:        return "False";
        case VTX_OP_POP:          return "Pop";
        case VTX_OP_GETLOCAL:     return "GetLocal";
        case VTX_OP_GETGLOBAL:    return "GetGlobal";
        case VTX_OP_DEFINEGLOBAL: return "DefineGlobal";
        case VTX_OP_SETLOCAL:     return "SetLocal";
        case VTX_OP_SETGLOBAL:    return "SetGlobal";
        case VTX_OP_GETUPVALUE:   return "GetUpvalue";
        case VTX_OP_SETUPVALUE:   return "SetUpvalue";
        case VTX_OP_GETPROPERTY:  return "GetProperty";
        case VTX_OP_SETPROPERTY:  return "SetProperty";
        case VTX_OP_GETSUPER:     return "GetSuper";
        case VTX_OP_EQUAL:        return "Equal";
        case VTX_OP_GREATER:      return "Greater";
        case VTX_OP_LESS:         return "Less";
        case VTX_OP_ADD:          return "Add";
        case VTX_OP_SUBTRACT:     return "Subtract";
        case VTX_OP_MULTIPLY:     return "Multiply";
        case VTX_OP_DIVIDE:       return "Divide";
        case VTX_OP_NOT:          return "Not";
        case VTX_OP_NEGATE:       return "Negate";
        case VTX_OP_PRINT:        return "Print";
        case VTX_OP_JUMP:         return "Jump";
        case VTX_OP_JUMPZERO:     return "JumpZero";
        case VTX_OP_LOOP:         return "Loop";
        case VTX_OP_CALL:         return "Call";
        case VTX_OP_INVOKE:       return "Invoke";
        case VTX_OP_SUPERINVOKE:  return "SuperInvoke";
        case VTX_OP_CLOSURE:      return "Closure";
        case VTX_OP_CLOSEUPVALUE: return "CloseUpvalue";
        case VTX_OP_RETURN:       return "Return";
        case VTX_OP_CLASS:        return "Class";
        case VTX_OP_INHERIT:      return "Inherit";
        case VTX_OP_METHOD:       return "Method";
        case VTX_OP_BWAND:        return "BwAnd";
        case VTX_OP_BWOR:         return "BwOr";
        case VTX_OP_BWXOR:        return "BwXor";
        case VTX_OP_BWNOT:        return "BwNot";
        case VTX_OP_SHIFTLEFT:    return "ShiftLeft";
        case VTX_OP_SHIFTRIGHT:   return "ShiftRight";
        case VTX_OP_ARRAYBUILD:   return "ArrayBuild";
        case VTX_OP_ARRAYINDEX:   return "ArrayIndex";
        case VTX_OP_ARRAYSTORE:   return "ArrayStore";
        case VTX_OP_IMPORT:       return "Import";
    }
    return "Unknown";
}

#endif
