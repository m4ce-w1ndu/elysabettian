//
//  Value.cpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#include "Value.hpp"

void Chunk::Write(uint8_t byte, int line)
{
    code.push_back(byte);
    lines.push_back(line);
}

void Chunk::Write(OpCode opcode, int line)
{
    Write(static_cast<uint8_t>(opcode), line);
}

unsigned long Chunk::AddConstant(Value value)
{
    constants.push_back(value);
    return static_cast<unsigned long>(constants.size() - 1);
}

void Chunk::Disassemble(const std::string& name)
{
    std::cout << "== " << name << " ==" << std::endl;
    
    for (auto i = 0; i < static_cast<int>(code.size());)
        i = DisasIntruction(i);
}

static int SimpleInstruction(const std::string& name, int offset)
{
    std::cout << name << std::endl;
    return offset + 1;
}

static int ConstantInstruction(const std::string& name, const Chunk& chunk, int offset)
{
    auto constant = chunk.GetCode(offset + 1);
    printf("%-16s %4d '", name.c_str(), constant);
    std::cout << chunk.GetConstant(constant);
    printf("'\n");
    return offset + 2;
}

static int InvokeInstruction(const std::string& name, const Chunk& chunk, int offset)
{
    auto constant = chunk.GetCode(offset + 1);
    auto argCount = chunk.GetCode(offset + 2);
    printf("%-16s (%d args) %4d '", name.c_str(), argCount, constant);
    std::cout << chunk.GetConstant(constant) << "'" << std::endl;
    return offset + 3;
}

static int ByteInstruction(const std::string& name, const Chunk& chunk, int offset)
{
    auto slot = chunk.GetCode(offset + 1);
    printf("%-16s %4d\n", name.c_str(), slot);
    return offset + 2;
}

static int JmpInstruction(const std::string& name, int sign, const Chunk& chunk, int offset)
{
    uint16_t jump = static_cast<uint16_t>(chunk.GetCode(offset + 1) << 8);
    jump |= static_cast<uint16_t>(chunk.GetCode(offset + 2));
    printf("%-16s %4d -> %d\n", name.c_str(), offset, offset + 3 + sign * jump);
    return offset + 3;
}

int Chunk::DisasIntruction(int offset)
{
    printf("%04d ", offset);
    
    if (offset > 0 && lines[offset] == lines[offset - 1]) {
        std::cout << "   | ";
    } else {
        printf("%4d ", lines[offset]);
    }
    
    auto instruction = OpCode(code[offset]);
    switch (instruction) {
        case OpCode::CONSTANT:
            return ConstantInstruction("OP_CONSTANT", *this, offset);
        case OpCode::NULLOP:
            return SimpleInstruction("OP_NULL", offset);
        case OpCode::TRUE:
            return SimpleInstruction("OP_TRUE", offset);
        case OpCode::FALSE:
            return SimpleInstruction("OP_FALSE", offset);
        case OpCode::POP:
            return SimpleInstruction("OP_POP", offset);
        case OpCode::GET_LOCAL:
            return ByteInstruction("OP_GET_LOCAL", *this, offset);
        case OpCode::GET_GLOBAL:
            return ConstantInstruction("OP_GET_GLOBAL", *this, offset);
        case OpCode::DEFINE_GLOBAL:
            return ConstantInstruction("OP_DEFINE_GLOBAL", *this, offset);
        case OpCode::SET_LOCAL:
            return ByteInstruction("OP_SET_LOCAL", *this, offset);
        case OpCode::SET_GLOBAL:
            return ConstantInstruction("OP_SET_GLOBAL", *this, offset);
        case OpCode::GET_UPVALUE:
            return ByteInstruction("OP_GET_UPVALUE", *this, offset);
        case OpCode::SET_UPVALUE:
            return ByteInstruction("OP_SET_UPVALUE", *this, offset);
        case OpCode::GET_PROPERTY:
            return ConstantInstruction("OP_GET_PROPERTY", *this, offset);
        case OpCode::SET_PROPERTY:
            return ConstantInstruction("OP_SET_PROPERTY", *this, offset);
        case OpCode::GET_SUPER:
            return ConstantInstruction("OP_GET_SUPER", *this, offset);
        case OpCode::EQUAL:
            return SimpleInstruction("OP_EQUAL", offset);
        case OpCode::GREATER:
            return SimpleInstruction("OP_GREATER", offset);
        case OpCode::LESS:
            return SimpleInstruction("OP_LESS", offset);
        case OpCode::ADD:
            return SimpleInstruction("OP_ADD", offset);
        case OpCode::SUBTRACT:
            return SimpleInstruction("OP_SUBTRACT", offset);
        case OpCode::MULTIPLY:
            return SimpleInstruction("OP_MULTIPLY", offset);
        case OpCode::DIVIDE:
            return SimpleInstruction("OP_DIVIDE", offset);
        case OpCode::BW_AND:
            return SimpleInstruction("OP_BW_AND", offset);
        case OpCode::BW_OR:
            return SimpleInstruction("OP_BW_OR", offset);
        case OpCode::BW_XOR:
            return SimpleInstruction("OP_BW_XOR", offset);
        case OpCode::BW_NOT:
            return SimpleInstruction("OP_BW_NOT", offset);
        case OpCode::NOT:
            return SimpleInstruction("OP_NOT", offset);
        case OpCode::NEGATE:
            return SimpleInstruction("OP_NEGATE", offset);
        case OpCode::PRINT:
            return SimpleInstruction("OP_PRINT", offset);
        case OpCode::JUMP:
            return JmpInstruction("OP_JUMP", 1, *this, offset);
        case OpCode::JUMP_IF_FALSE:
            return JmpInstruction("OP_JUMP_IF_FALSE", 1, *this, offset);
        case OpCode::LOOP:
            return JmpInstruction("OP_JUMP", -1, *this, offset);
        case OpCode::CALL:
            return ByteInstruction("OP_CALL", *this, offset);
        case OpCode::INVOKE:
            return InvokeInstruction("OP_INVOKE", *this, offset);
        case OpCode::SUPER_INVOKE:
            return InvokeInstruction("OP_SUPER_INVOKE", *this, offset);
        case OpCode::CLOSURE: {
            offset++;
            auto constant = code[offset++];
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            std::cout << constants[constant];
            std::cout << std::endl;
            
            auto function = std::get<Func>(constants[constant]);
            for (int j = 0; j < function->upvalueCount; j++) {
                int isLocal = code[offset++];
                int index = code[offset++];
                printf("%04d      |                     %s %d\n",
                       offset - 2, isLocal ? "local" : "upvalue", index);
            }
            
            return offset;
        }
        case OpCode::CLOSE_UPVALUE:
            return SimpleInstruction("OP_CLOSE_UPVALUE", offset);
        case OpCode::RETURN:
            return SimpleInstruction("OP_RETURN", offset);
        case OpCode::CLASS:
            return ConstantInstruction("OP_CLASS", *this, offset);
        case OpCode::INHERIT:
            return SimpleInstruction("OP_INHERIT", offset);
        case OpCode::METHOD:
            return ConstantInstruction("OP_METHOD", *this, offset);
    }
    
    std::cout << "Unknown opcode: " << code[offset] << std::endl;
    return offset + 1;
}
