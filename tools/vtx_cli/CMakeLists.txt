add_executable(vtx_cli)

target_include_directories(vtx_cli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(vtx_cli
    PRIVATE
        vtx::core
        vtx::compiler
        vtx::stdlib
        vtx::vm
        vtx::jit
)

vtx_enable_warnings(vtx_cli)
