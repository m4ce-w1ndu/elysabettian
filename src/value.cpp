#include "runtime/value.h"

/// @brief Writes code into the chunk
/// @param byte byte of code to write
/// @param line line bound to this byte
void Chunk::write(uint8_t byte, int line)
{
    code.push_back(byte);
    lines.push_back(line);
}

void Chunk::write(Opcode opcode, int line)
{
    write(static_cast<uint8_t>(opcode), line);
}

size_t Chunk::add_constant(Value value)
{
    constants.push_back(value);
    return constants.size() - 1;
}

void Chunk::disassemble(const std::string& name)
{
    fmt::print("== {} ==\n", name);
    
    for (int i = 0; i < static_cast<int>(code.size());)
        i = disas_instruction(i);
}

static int simple_instruction(const std::string& name, int offset)
{
    fmt::print("{}\n", name);
    return offset + 1;
}

static int constant_instruction(const std::string& name, const Chunk& chunk, int offset)
{
    uint8_t constant = chunk.get_code(offset + 1);
	fmt::print("{:<16s} {:4d} '", name, constant);
    std::cout << chunk.get_constant(constant);
    fmt::print("'\n");
    return offset + 2;
}

static int invoke_instruction(const std::string& name, const Chunk& chunk, int offset)
{
    uint8_t constant = chunk.get_code(offset + 1);
    uint8_t arg_count = chunk.get_code(offset + 2);
	fmt::print("{:<16s} ({} args) {:4d} '", name, arg_count, constant);
    std::cout << chunk.get_constant(constant) << "'\n";
    return offset + 3;
}

static int byte_instruction(const std::string& name, const Chunk& chunk, int offset)
{
    uint8_t slot = chunk.get_code(offset + 1);
    fmt::print("{:<16s} {:4d}\n", name, slot);
    return offset + 2;
}

static int jmp_instruction(const std::string& name, int sign, const Chunk& chunk, int offset)
{
    uint16_t jump = static_cast<uint16_t>(chunk.get_code(offset + 1) << 8);
    jump |= static_cast<uint16_t>(chunk.get_code(offset + 2));
    fmt::print("{:<16s} {:4d} -> {}\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int Chunk::disas_instruction(int offset)
{
    fmt::print("{:04d} ", offset);   
    if (offset > 0 && lines[offset] == lines[offset - 1]) {
        fmt::print("   | ");
    } else {
        fmt::print("{:4d} ", lines[offset]);
    }
    
    Opcode instruction = Opcode(code[offset]);
    switch (instruction) {
        case Opcode::Constant:
            return constant_instruction("OP_CONSTANT", *this, offset);
        case Opcode::Nop:
            return simple_instruction("OP_NULL", offset);
        case Opcode::True:
            return simple_instruction("OP_TRUE", offset);
        case Opcode::False:
            return simple_instruction("OP_FALSE", offset);
        case Opcode::Pop:
            return simple_instruction("OP_POP", offset);
        case Opcode::GetLocal:
            return byte_instruction("OP_GET_LOCAL", *this, offset);
        case Opcode::GetGlobal:
            return constant_instruction("OP_GET_GLOBAL", *this, offset);
        case Opcode::DefineGlobal:
            return constant_instruction("OP_DEFINE_GLOBAL", *this, offset);
        case Opcode::SetLocal:
            return byte_instruction("OP_SET_LOCAL", *this, offset);
        case Opcode::SetGlobal:
            return constant_instruction("OP_SET_GLOBAL", *this, offset);
        case Opcode::GetUpvalue:
            return byte_instruction("OP_GET_UPVALUE", *this, offset);
        case Opcode::SetUpvalue:
            return byte_instruction("OP_SET_UPVALUE", *this, offset);
        case Opcode::GetProperty:
            return constant_instruction("OP_GET_PROPERTY", *this, offset);
        case Opcode::SetProperty:
            return constant_instruction("OP_SET_PROPERTY", *this, offset);
        case Opcode::GetSuper:
            return constant_instruction("OP_GET_SUPER", *this, offset);
        case Opcode::Equal:
            return simple_instruction("OP_EQUAL", offset);
        case Opcode::Greater:
            return simple_instruction("OP_GREATER", offset);
        case Opcode::Less:
            return simple_instruction("OP_LESS", offset);
        case Opcode::Add:
            return simple_instruction("OP_ADD", offset);
        case Opcode::Subtract:
            return simple_instruction("OP_SUBTRACT", offset);
        case Opcode::Multiply:
            return simple_instruction("OP_MULTIPLY", offset);
        case Opcode::Divide:
            return simple_instruction("OP_DIVIDE", offset);
        case Opcode::BwAnd:
            return simple_instruction("OP_BW_AND", offset);
        case Opcode::BwOr:
            return simple_instruction("OP_BW_OR", offset);
        case Opcode::BwXor:
            return simple_instruction("OP_BW_XOR", offset);
        case Opcode::BwNot:
            return simple_instruction("OP_BW_NOT", offset);
        case Opcode::Not:
            return simple_instruction("OP_NOT", offset);
        case Opcode::Negate:
            return simple_instruction("OP_NEGATE", offset);
        case Opcode::Print:
            return simple_instruction("OP_PRINT", offset);
        case Opcode::Jump:
            return jmp_instruction("OP_JUMP", 1, *this, offset);
        case Opcode::JumpIfFalse:
            return jmp_instruction("OP_JUMP_IF_FALSE", 1, *this, offset);
        case Opcode::Loop:
            return jmp_instruction("OP_JUMP", -1, *this, offset);
        case Opcode::Call:
            return byte_instruction("OP_CALL", *this, offset);
        case Opcode::Invoke:
            return invoke_instruction("OP_INVOKE", *this, offset);
        case Opcode::SuperInvoke:
            return invoke_instruction("OP_SUPER_INVOKE", *this, offset);
        case Opcode::Closure: {
            offset++;
            uint8_t constant = code[offset++];
            fmt::print("{:<16s} {:4d}", "OP_CLOSURE", constant);
            std::cout << constants[constant];
            std::cout << std::endl;
            
            Func function = std::get<Func>(constants[constant]);
            for (int j = 0; j < function->upvalue_count; j++) {
                int is_local = code[offset++];
                int index = code[offset++];
                fmt::print("{:04d}      |                     {} {}\n",
                    offset - 2, is_local ? "local" : "upvalue", index);
            }
            
            return offset;
        }
        case Opcode::CloseUpvalue:
            return simple_instruction("OP_CLOSE_UPVALUE", offset);
        case Opcode::Return:
            return simple_instruction("OP_RETURN", offset);
        case Opcode::Class:
            return constant_instruction("OP_CLASS", *this, offset);
        case Opcode::Inherit:
            return simple_instruction("OP_INHERIT", offset);
        case Opcode::Method:
            return constant_instruction("OP_METHOD", *this, offset);
        case Opcode::ArrBuild:
            return constant_instruction("ARR_BUILD", *this, offset);
        case Opcode::ArrIndex:
            return simple_instruction("ARR_INDEX", offset);
        case Opcode::ArrStore:
            return constant_instruction("ARR_STORE", *this, offset);
    }
    
	fmt::print("Uknown opcode: {}\n", code[offset]);
    return offset + 1;
}
