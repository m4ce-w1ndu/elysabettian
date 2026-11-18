add_library(vtx_vm)
add_library(vtx::vm ALIAS vtx_vm)

target_include_directories(vtx_vm
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(vtx_vm
    PUBLIC
        vtx::core
        vtx::compiler
        vtx::stdlib
        # vtx::jit
)

target_sources(vtx_vm
    PRIVATE
        src/heap.c
        src/vm.c
)

vtx_enable_warnings(vtx_vm)
