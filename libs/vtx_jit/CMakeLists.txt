add_library(vtx_jit)
add_library(vtx::jit ALIAS vtx_jit)

target_include_directories(vtx_jit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(vtx_jit
    PUBLIC
        vtx::core
)

vtx_enable_warnings(vtx_jit)
