#include "value.hpp"

void Chunk::write(uint8_t byte, int line)
{
    code.push_back(byte);
    lines.push_back(line);
}

void Chunk::write(Opcode opcode, int line)
{
    write(static_cast<uint8_t>(opcode), line);
}

unsigned long Chunk::add_constant(Value value)
{
    constants.push_back(value);
    return static_cast<unsigned long>(constants.size() - 1);
}

void Chunk::disassemble(const std::string& name)
{
    fmt::print("== {} ==\n", name);
    
    for (auto i = 0; i < static_cast<int>(code.size());)
        i = disas_instruction(i);
}

static int SimpleInstruction(const std::string& name, int offset)
{
    fmt::print("{}\n", name);
    return offset + 1;
}

static int ConstantInstruction(const std::string& name, const Chunk& chunk, int offset)
{
    auto constant = chunk.get_code(offset + 1);
	fmt::print("{:<16s} {:4d} '", name, constant);
    std::cout << chunk.get_constant(constant);
    fmt::print("'\n");
    return offset + 2;
}

static int InvokeInstruction(const std::string& name, const Chunk& chunk, int offset)
{
    auto constant = chunk.get_code(offset + 1);
    auto arg_count = chunk.get_code(offset + 2);
	fmt::print("{:<16s} ({} args) {:4d} '", name, arg_count, constant);
    std::cout << chunk.get_constant(constant) << "'\n";
    return offset + 3;
}

static int ByteInstruction(const std::string& name, const Chunk& chunk, int offset)
{
    auto slot = chunk.get_code(offset + 1);
    fmt::print("{:<16s} {:4d}\n", name, slot);
    return offset + 2;
}

static int JmpInstruction(const std::string& name, int sign, const Chunk& chunk, int offset)
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
    
    auto instruction = Opcode(code[offset]);
    switch (instruction) {
        case Opcode::Constant:
            return ConstantInstruction("OP_CONSTANT", *this, offset);
        case Opcode::Nop:
            return SimpleInstruction("OP_NULL", offset);
        case Opcode::True:
            return SimpleInstruction("OP_TRUE", offset);
        case Opcode::False:
            return SimpleInstruction("OP_FALSE", offset);
        case Opcode::Pop:
            return SimpleInstruction("OP_POP", offset);
        case Opcode::GetLocal:
            return ByteInstruction("OP_GET_LOCAL", *this, offset);
        case Opcode::GetGlobal:
            return ConstantInstruction("OP_GET_GLOBAL", *this, offset);
        case Opcode::DefineGlobal:
            return ConstantInstruction("OP_DEFINE_GLOBAL", *this, offset);
        case Opcode::SetLocal:
            return ByteInstruction("OP_SET_LOCAL", *this, offset);
        case Opcode::SetGlobal:
            return ConstantInstruction("OP_SET_GLOBAL", *this, offset);
        case Opcode::GetUpvalue:
            return ByteInstruction("OP_GET_UPVALUE", *this, offset);
        case Opcode::SetUpvalue:
            return ByteInstruction("OP_SET_UPVALUE", *this, offset);
        case Opcode::GetProperty:
            return ConstantInstruction("OP_GET_PROPERTY", *this, offset);
        case Opcode::SetProperty:
            return ConstantInstruction("OP_SET_PROPERTY", *this, offset);
        case Opcode::GetSuper:
            return ConstantInstruction("OP_GET_SUPER", *this, offset);
        case Opcode::Equal:
            return SimpleInstruction("OP_EQUAL", offset);
        case Opcode::Greater:
            return SimpleInstruction("OP_GREATER", offset);
        case Opcode::Less:
            return SimpleInstruction("OP_LESS", offset);
        case Opcode::Add:
            return SimpleInstruction("OP_ADD", offset);
        case Opcode::Subtract:
            return SimpleInstruction("OP_SUBTRACT", offset);
        case Opcode::Multiply:
            return SimpleInstruction("OP_MULTIPLY", offset);
        case Opcode::Divide:
            return SimpleInstruction("OP_DIVIDE", offset);
        case Opcode::BwAnd:
            return SimpleInstruction("OP_BW_AND", offset);
        case Opcode::BwOr:
            return SimpleInstruction("OP_BW_OR", offset);
        case Opcode::BwXor:
            return SimpleInstruction("OP_BW_XOR", offset);
        case Opcode::BwNot:
            return SimpleInstruction("OP_BW_NOT", offset);
        case Opcode::Not:
            return SimpleInstruction("OP_NOT", offset);
        case Opcode::Negate:
            return SimpleInstruction("OP_NEGATE", offset);
        case Opcode::Print:
            return SimpleInstruction("OP_PRINT", offset);
        case Opcode::Jump:
            return JmpInstruction("OP_JUMP", 1, *this, offset);
        case Opcode::JumpIfFalse:
            return JmpInstruction("OP_JUMP_IF_FALSE", 1, *this, offset);
        case Opcode::Loop:
            return JmpInstruction("OP_JUMP", -1, *this, offset);
        case Opcode::Call:
            return ByteInstruction("OP_CALL", *this, offset);
        case Opcode::Invoke:
            return InvokeInstruction("OP_INVOKE", *this, offset);
        case Opcode::SuperInvoke:
            return InvokeInstruction("OP_SUPER_INVOKE", *this, offset);
        case Opcode::Closure: {
            offset++;
            auto constant = code[offset++];
            fmt::print("{:<16s} {:4d}", "OP_CLOSURE", constant);
            std::cout << constants[constant];
            std::cout << std::endl;
            
            auto function = std::get<Func>(constants[constant]);
            for (int j = 0; j < function->upvalue_count; j++) {
                int is_local = code[offset++];
                int index = code[offset++];
                fmt::print("{:04d}      |                     {} {}\n",
                    offset - 2, is_local ? "local" : "upvalue", index);
            }
            
            return offset;
        }
        case Opcode::CloseUpvalue:
            return SimpleInstruction("OP_CLOSE_UPVALUE", offset);
        case Opcode::Return:
            return SimpleInstruction("OP_RETURN", offset);
        case Opcode::Class:
            return ConstantInstruction("OP_CLASS", *this, offset);
        case Opcode::Inherit:
            return SimpleInstruction("OP_INHERIT", offset);
        case Opcode::Method:
            return ConstantInstruction("OP_METHOD", *this, offset);
    }
    
	fmt::print("Uknown opcode: {}\n", code[offset]);
    return offset + 1;
}
