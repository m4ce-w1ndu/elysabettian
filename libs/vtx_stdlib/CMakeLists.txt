add_library(vtx_stdlib)
add_library(vtx::stdlib ALIAS vtx_stdlib)

target_include_directories(vtx_stdlib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(vtx_stdlib
    PUBLIC
        vtx::core
)

target_sources(vtx_stdlib
    PRIVATE
        src/table.c
)

vtx_enable_warnings(vtx_stdlib)
