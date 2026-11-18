add_executable(vtx_tests
    src/test_main.c
    src/test_core.c
    src/test_scanner.c
    src/test_table.c
)

target_include_directories(vtx_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(vtx_tests
    PRIVATE
        vtx::core
        vtx::compiler
        vtx::stdlib
)

vtx_enable_warnings(vtx_tests)

add_test(NAME vtx_tests COMMAND vtx_tests)
