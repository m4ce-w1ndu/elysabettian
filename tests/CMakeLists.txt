include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)

FetchContent_MakeAvailable(Catch2)

add_executable(ely_tests
    src/test_main.cpp
    src/test_core.cpp
    src/test_scanner.cpp
    src/test_compiler.cpp
    src/test_vm.cpp
    src/test_stdlib.cpp
    src/test_jit.cpp
)

target_link_libraries(ely_tests
    PRIVATE
        Catch2::Catch2WithMain
        ely::core
        ely::compiler
        ely::vm
        ely::stdlib
        ely::jit
)

get_target_property(ASMJIT_INCLUDE_DIRS asmjit INTERFACE_INCLUDE_DIRECTORIES)
if(ASMJIT_INCLUDE_DIRS)
    target_include_directories(ely_tests SYSTEM PRIVATE ${ASMJIT_INCLUDE_DIRS})
endif()

ely_enable_warnings(ely_tests)

include(CTest)
include(Catch)
catch_discover_tests(ely_tests)
