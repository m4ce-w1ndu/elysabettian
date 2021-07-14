cmake_minimum_required(VERSION 3.10)
project(CElysabettian CXX)

set(EXEC cely)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)

if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /MTd /Z7")
    set(CMAKE_CXX_FLAGS_RELEASE "/Os")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -O0 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os")
endif()

add_executable(${EXEC}
    "src/Application/Main.cpp" "src/Application/Common.hpp" "src/ExecEnv/CommonLibrary.hpp"
    "src/ExecEnv/Compiler.hpp" "src/ExecEnv/Compiler.cpp" "src/ExecEnv/Library.cpp"
    "src/ExecEnv/Math.hpp" "src/ExecEnv/Math.cpp" "src/ExecEnv/String.hpp"
    "src/ExecEnv/String.cpp" "src/ExecEnv/Opcodes.hpp" "src/ExecEnv/Value.hpp"
    "src/ExecEnv/Value.cpp" "src/ExecEnv/VirtualMachine.hpp" "src/ExecEnv/VirtualMachine.cpp"
    "src/Lexer/Tokenizer.hpp" "src/Lexer/Tokenizer.cpp" "src/ExecEnv/IoFile.hpp" "src/ExecEnv/IoFile.cpp")