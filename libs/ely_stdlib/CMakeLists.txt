add_library(ely_stdlib)
add_library(ely::stdlib ALIAS ely_stdlib)

target_include_directories(ely_stdlib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ely_stdlib
    PUBLIC
        ely::core
    PRIVATE
        ${CMAKE_DL_LIBS}
)

target_sources(ely_stdlib
    PRIVATE
        src/stdlib.cpp
        src/math.cpp
        src/system.cpp
        src/registry.cpp
)

ely_enable_warnings(ely_stdlib)
