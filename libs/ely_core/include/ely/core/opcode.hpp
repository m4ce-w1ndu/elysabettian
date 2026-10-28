#pragma once

#include <cstdint>
#include <string_view>

namespace ely::core {

/**
 * @brief Bytecode Instruction Set Architecture (ISA) for the Elysabettian VM.
 *
 * Each opcode is an 8-bit unsigned integer (uint8_t) executed by the stack-based
 * interpreter or lowered to native machine code by the JIT compiler.
 */
enum struct Opcode : uint8_t {
    // Stack and constants
    /// Pushes constants[const_index] onto the stack. Operand: [const_index: u8].
    Constant = 0,
    /// No operation.
    Nop,
    /// Pushes boolean true onto the stack.
    True,
    /// Pushes boolean false onto the stack.
    False,
    /// Discards the top stack value: [..., value] -> [...].
    Pop,

    // Variables and bindings
    /// Reads local variable from current call frame. Operand: [slot: u8].
    GetLocal,
    /// Looks up global variable by constant name string. Operand: [name_index: u8].
    GetGlobal,
    /// Defines and binds a global variable. Operand: [name_index: u8].
    DefineGlobal,
    /// Updates local slot and leaves value on stack. Operand: [slot: u8].
    SetLocal,
    /// Updates existing global variable and leaves value on stack. Operand: [name_index: u8].
    SetGlobal,
    /// Reads captured upvalue from closure environment. Operand: [upvalue_index: u8].
    GetUpvalue,
    /// Writes to captured upvalue in closure environment. Operand: [upvalue_index: u8].
    SetUpvalue,
    /// Accesses an object field or binds a method. Operand: [name_index: u8].
    GetProperty,
    /// Sets a field on an instance. Operand: [name_index: u8].
    SetProperty,
    /// Resolves superclass method using current `this`. Operand: [name_index: u8].
    GetSuper,

    // Comparisons
    /// Compares equality between top two values: [..., a, b] -> [..., a == b].
    Equal,
    /// Greater-than comparison: [..., a, b] -> [..., a > b].
    Greater,
    /// Less-than comparison: [..., a, b] -> [..., a < b].
    Less,

    // Arithmetic and logical operations
    /// Numeric addition or string concatenation: [..., a, b] -> [..., a + b].
    Add,
    /// Subtraction: [..., a, b] -> [..., a - b].
    Subtract,
    /// Multiplication: [..., a, b] -> [..., a * b].
    Multiply,
    /// Division: [..., a, b] -> [..., a / b].
    Divide,
    /// Logical NOT: [..., value] -> [..., !truthy(value)].
    Not,
    /// Arithmetic negation: [..., number] -> [..., -number].
    Negate,

    // Control flow and output
    /// Pops top value and prints it to stdout with a newline.
    Print,
    /// Unconditional forward jump. Operands: [hi: u8, lo: u8] (16-bit offset).
    Jump,
    /// Conditional forward jump if condition is false/zero. Operands: [hi: u8, lo: u8].
    JumpZero,
    /// Unconditional backward jump. Operands: [hi: u8, lo: u8] (16-bit offset).
    Loop,

    // Functions and closures
    /// Invokes function/closure/class constructor. Operand: [arg_count: u8].
    Call,
    /// Fused method lookup and call optimization. Operands: [name_idx: u8, argc: u8].
    Invoke,
    /// Fused superclass method invocation. Operands: [name_idx: u8, argc: u8].
    SuperInvoke,
    /// Instantiates a closure capturing upvalues. Operands: [fn_idx: u8, (is_local, index)...].
    Closure,
    /// Closes an open upvalue on stack and pops it.
    CloseUpvalue,
    /// Returns from current call frame to caller.
    Return,

    // Classes and inheritance
    /// Creates a new class descriptor. Operand: [name_index: u8].
    Class,
    /// Copies methods from superclass to subclass: [..., super, sub] -> [..., super, sub].
    Inherit,
    /// Binds closure as a method on target class. Operand: [name_index: u8].
    Method,

    // Bitwise operations
    /// Bitwise AND: [..., a, b] -> [..., a & b].
    BwAnd,
    /// Bitwise OR: [..., a, b] -> [..., a | b].
    BwOr,
    /// Bitwise XOR: [..., a, b] -> [..., a ^ b].
    BwXor,
    /// Bitwise NOT: [..., value] -> [..., ~value].
    BwNot,
    /// Bitwise shift left: [..., a, b] -> [..., a << b].
    ShiftLeft,
    /// Bitwise shift right: [..., a, b] -> [..., a >> b].
    ShiftRight,

    // Arrays
    /// Creates an array with top `count` values. Operand: [count: u8].
    ArrayBuild,
    /// Subscript index read: [..., array, index] -> [..., element].
    ArrayIndex,
    /// Subscript index write: [..., array, index, val] -> [..., val].
    ArrayStore,

    // Modules
    /// Dynamically imports and binds a module. Operand: [name_index: u8].
    Import,
};

/**
 * @brief Returns a human-readable string representation of an opcode.
 */
[[nodiscard]] constexpr std::string_view opcode_name(Opcode op) noexcept {
    switch (op) {
        case Opcode::Constant:     return "Constant";
        case Opcode::Nop:          return "Nop";
        case Opcode::True:         return "True";
        case Opcode::False:        return "False";
        case Opcode::Pop:          return "Pop";
        case Opcode::GetLocal:     return "GetLocal";
        case Opcode::GetGlobal:    return "GetGlobal";
        case Opcode::DefineGlobal: return "DefineGlobal";
        case Opcode::SetLocal:     return "SetLocal";
        case Opcode::SetGlobal:    return "SetGlobal";
        case Opcode::GetUpvalue:   return "GetUpvalue";
        case Opcode::SetUpvalue:   return "SetUpvalue";
        case Opcode::GetProperty:  return "GetProperty";
        case Opcode::SetProperty:  return "SetProperty";
        case Opcode::GetSuper:     return "GetSuper";
        case Opcode::Equal:        return "Equal";
        case Opcode::Greater:      return "Greater";
        case Opcode::Less:         return "Less";
        case Opcode::Add:          return "Add";
        case Opcode::Subtract:     return "Subtract";
        case Opcode::Multiply:     return "Multiply";
        case Opcode::Divide:       return "Divide";
        case Opcode::Not:          return "Not";
        case Opcode::Negate:       return "Negate";
        case Opcode::Print:        return "Print";
        case Opcode::Jump:         return "Jump";
        case Opcode::JumpZero:     return "JumpZero";
        case Opcode::Loop:         return "Loop";
        case Opcode::Call:         return "Call";
        case Opcode::Invoke:       return "Invoke";
        case Opcode::SuperInvoke:  return "SuperInvoke";
        case Opcode::Closure:      return "Closure";
        case Opcode::CloseUpvalue: return "CloseUpvalue";
        case Opcode::Return:       return "Return";
        case Opcode::Class:        return "Class";
        case Opcode::Inherit:      return "Inherit";
        case Opcode::Method:       return "Method";
        case Opcode::BwAnd:        return "BwAnd";
        case Opcode::BwOr:         return "BwOr";
        case Opcode::BwXor:        return "BwXor";
        case Opcode::BwNot:        return "BwNot";
        case Opcode::ShiftLeft:    return "ShiftLeft";
        case Opcode::ShiftRight:   return "ShiftRight";
        case Opcode::ArrayBuild:   return "ArrayBuild";
        case Opcode::ArrayIndex:   return "ArrayIndex";
        case Opcode::ArrayStore:   return "ArrayStore";
        case Opcode::Import:       return "Import";
    }
    return "Unknown";
}

} // namespace ely::core
