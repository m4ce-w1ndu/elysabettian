add_executable(ely_tests
    src/test_main.c
    src/test_core.c
    src/test_scanner.c
    src/test_table.c
)

target_include_directories(ely_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ely_tests
    PRIVATE
        ely::core
        ely::compiler
        ely::stdlib
)

ely_enable_warnings(ely_tests)

add_test(NAME ely_tests COMMAND ely_tests)
