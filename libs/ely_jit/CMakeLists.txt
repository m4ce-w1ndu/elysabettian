add_library(ely_jit)
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
        ely::core
)

ely_enable_warnings(ely_jit)
