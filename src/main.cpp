#include <fstream>
#include <cstring>

#include "common.h"
#include "runtime/value.h"
#include "runtime/core_vm.h"

static void Repl(VirtualMachine& vm)
{
    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        vm.interpret(line);
        
        if (std::cin.eof()) {
            std::cout << std::endl;
            break;
        }
    }
}

static std::string ReadFile(const std::string& path)
{
    std::ifstream t(path);
    std::string str;
    
    t.seekg(0, std::ios::end);
    str.reserve(t.tellg());
    t.seekg(0, std::ios::beg);
    
    str.assign((std::istreambuf_iterator<char>(t)),
               std::istreambuf_iterator<char>());
    
    return str;
}

static void RunFile(VirtualMachine& vm, const std::string& path)
{
    auto source = ReadFile(path);
    auto result = vm.interpret(source);
    
    switch (result) {
        case InterpretResult::Ok: break;
        case InterpretResult::CompileError: exit(65);
        case InterpretResult::RuntimeError: exit(70);
    }
}

static void RunCommand(VirtualMachine& vm, const std::string& command)
{
    vm.interpret(command);
}

int main(int argc, const char * argv[])
{
    auto vm = VirtualMachine();
    
    if (argc == 1) {
        Repl(vm);
    } else if (argc == 2) {
        RunFile(vm, argv[1]);
    } else if (argc == 3 && strcmp(argv[1], "-c") == 0) {
        RunCommand(vm, argv[2]);
    } else {
        std::cerr << "Usage: cely [PATH_TO_SCRIPT]" << std::endl;
        exit(64);
    }
    
    return 0;
}
