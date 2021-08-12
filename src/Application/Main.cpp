//
//  Main.cpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#include <fstream>
#include <cstring>

#include "Common.hpp"
#include "../ExecEnv/Value.hpp"
#include "../ExecEnv/VirtualMachine.hpp"

static void Repl(VM& vm)
{
    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        vm.Interpret(line);
        
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

static void RunFile(VM& vm, const std::string& path)
{
    auto source = ReadFile(path);
    auto result = vm.Interpret(source);
    
    switch (result) {
        case IResult::OK: break;
        case IResult::COMPILE_ERROR: exit(65);
        case IResult::RUNTIME_ERROR: exit(70);
    }
}

static void RunCommand(VM& vm, const std::string& command)
{
    vm.Interpret(command);
}

int main(int argc, const char * argv[])
{
    auto vm = VM();
    
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
