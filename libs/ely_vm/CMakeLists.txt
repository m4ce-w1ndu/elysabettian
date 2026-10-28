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
)

target_sources(ely_vm
    PRIVATE
        # Add your .cpp files here (e.g. src/vm.cpp, src/gc.cpp)
)

ely_enable_warnings(ely_vm)
