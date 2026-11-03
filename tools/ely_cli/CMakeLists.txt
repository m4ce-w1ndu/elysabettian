add_executable(ely_cli)

target_include_directories(ely_cli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ely_cli
    PRIVATE
        ely::core
        ely::compiler
        ely::stdlib
        ely::vm
        ely::jit
)

target_sources(ely_cli
    PRIVATE
        src/main.cpp
        src/repl.cpp
        src/runner.cpp
)

ely_enable_warnings(ely_cli)
