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

file(GLOB appsrc "src/Application/*.cpp" "src/Application/*.hpp")
file(GLOB lexsrc "src/Lexer/*.cpp" "src/Lexer/*.hpp")
file(GLOB execenvsrc "src/ExecEnv/*.cpp" "src/ExecEnv/*.hpp")

file(GLOB_RECURSE src "*.cpp" "*.hpp")

add_executable(${EXEC} ${appsrc} ${lexsrc} ${execenvsrc} "src/ExecEnv/String.hpp" "src/ExecEnv/String.cpp")