add_library(ely_jit STATIC)
add_library(ely::jit ALIAS ely_jit)

target_include_directories(ely_jit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ely_jit
    PUBLIC
        ely::stdlib
    PUBLIC
        ely::core
        asmjit::asmjit
)

get_target_property(ASMJIT_INCLUDE_DIRS asmjit INTERFACE_INCLUDE_DIRECTORIES)
if(ASMJIT_INCLUDE_DIRS)
    target_include_directories(ely_jit SYSTEM PUBLIC ${ASMJIT_INCLUDE_DIRS})
endif()

target_sources(ely_jit
    PRIVATE
        src/runtime.cpp
        src/engine.cpp
        src/compiler.cpp
        src/compiler_x86.cpp
        src/compiler_a64.cpp
)

ely_enable_warnings(ely_jit)
