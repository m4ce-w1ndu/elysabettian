add_library(ely_vm)
add_library(ely::vm ALIAS ely_vm)

target_include_directories(ely_vm
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ely_vm
    PUBLIC
        ely::core
        ely::compiler
        ely::stdlib
        # ely::jit
)

target_sources(ely_vm
    PRIVATE
        src/heap.c
        src/vm.c
)

ely_enable_warnings(ely_vm)
