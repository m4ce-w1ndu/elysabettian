add_library(vtx_core)
add_library(vtx::core ALIAS vtx_core)

target_include_directories(vtx_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_sources(vtx_core
    PRIVATE
        src/chunk.c
        src/debug.c
)

vtx_enable_warnings(vtx_core)
