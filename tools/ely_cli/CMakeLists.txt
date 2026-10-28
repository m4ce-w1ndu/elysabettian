add_executable(ely_cli)

target_include_directories(ely_cli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ely_cli
    PRIVATE
        ely::core
        ely::compiler
        ely::stdlib
        ely::vm
)

target_sources(ely_cli
    PRIVATE
        # Add src/main.cpp here
)

ely_enable_warnings(ely_cli)
