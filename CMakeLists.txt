cmake_minimum_required(VERSION 3.25)
project(
    elysabettian
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "High-performance runtime for the Elysabettian language in modern C++"
)

# Standard C++20 requirement
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for LSP (clangd, ccls, IDEs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Global options
option(ELY_BUILD_TESTS "Build test suite" ON)
option(ELY_BUILD_TOOLS "Build CLI and developer tools" ON)

# Strict warnings and sanitizers helper function
function(ely_enable_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE 
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wnon-virtual-dtor
        )
    endif()
endfunction()

# Subdirectories for libraries
add_subdirectory(libs/ely_core)
add_subdirectory(libs/ely_compiler)
add_subdirectory(libs/ely_stdlib)
add_subdirectory(libs/ely_vm)

# Tools (CLI, REPL)
if(ELY_BUILD_TOOLS)
    add_subdirectory(tools/ely_cli)
endif()

# Tests
if(ELY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
