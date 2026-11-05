cmake_minimum_required(VERSION 3.25)
project(
    elysabettian_plainc
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "High-performance runtime for the Elysabettian language in modern C (C11)"
)

# Standard C11 requirement
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Export compile_commands.json for LSP (clangd, ccls, IDEs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
file(CREATE_LINK
    "${CMAKE_BINARY_DIR}/compile_commands.json"
    "${CMAKE_SOURCE_DIR}/compile_commands.json"
    SYMBOLIC COPY_ON_ERROR
)

# Global options
option(ELY_BUILD_TESTS "Build test suite" ON)
option(ELY_BUILD_TOOLS "Build CLI and developer tools" ON)

# Strict warnings helper function
function(ely_enable_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wmissing-prototypes -Wstrict-prototypes
        )
    endif()
endfunction()

# Subdirectories for libraries
add_subdirectory(libs/ely_core)
add_subdirectory(libs/ely_jit)
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
