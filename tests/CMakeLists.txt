add_executable(ely_tests)

target_link_libraries(ely_tests
    PRIVATE
        ely::core
        ely::compiler
        ely::vm
        ely::stdlib
        ely::jit
)

ely_enable_warnings(ely_tests)

include(CTest)
add_test(NAME ely_tests COMMAND ely_tests)
