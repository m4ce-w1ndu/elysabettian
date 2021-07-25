cmake_minimum_required(VERSION 3.10)
project(CElysabettian CXX)

set(EXEC cely)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)

if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /MTd /Z7")
    set(CMAKE_CXX_FLAGS_RELEASE "/Os")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -O0 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

include_directories(src)

add_executable(${EXEC}
    "src/Application/Main.cpp" "src/Application/Common.hpp"
    "src/ExecEnv/Compiler.hpp" "src/ExecEnv/Compiler.cpp"
    "src/ExecEnv/Opcodes.hpp" "src/ExecEnv/Value.hpp"
    "src/ExecEnv/Value.cpp" "src/ExecEnv/VirtualMachine.hpp" "src/ExecEnv/VirtualMachine.cpp"
    "src/Lexer/Tokenizer.hpp" "src/Lexer/Tokenizer.cpp" "src/Libraries/Library.hpp"
    "src/Libraries/Library.cpp" "src/Libraries/Math.cpp" "src/Libraries/StdIO.cpp"
    "src/Libraries/Array.cpp")