add_library(ely_compiler)
add_library(ely::compiler ALIAS ely_compiler)

target_include_directories(ely_compiler
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ely_compiler
    PUBLIC
        ely::core
)

target_sources(ely_compiler
    PRIVATE
        src/scanner.c
        src/parser.c
        src/scope.c
        src/compiler.c
        src/compiler_scope.c
        src/compiler_expr.c
        src/compiler_stmt.c
)

ely_enable_warnings(ely_compiler)
