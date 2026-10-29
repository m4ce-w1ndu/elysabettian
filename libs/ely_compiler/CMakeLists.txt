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
        src/scanner.cpp
        src/parser.cpp
        src/scope.cpp
        src/compiler.cpp
        src/compiler_scope.cpp
)

ely_enable_warnings(ely_compiler)
