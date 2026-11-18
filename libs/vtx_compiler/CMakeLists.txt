add_library(vtx_compiler)
add_library(vtx::compiler ALIAS vtx_compiler)

target_include_directories(vtx_compiler
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(vtx_compiler
    PUBLIC
        vtx::core
)

target_sources(vtx_compiler
    PRIVATE
        src/scanner.c
        src/parser.c
        src/scope.c
        src/compiler.c
        src/compiler_scope.c
        src/compiler_expr.c
        src/compiler_stmt.c
)

vtx_enable_warnings(vtx_compiler)
