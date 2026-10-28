add_library(ely_core)
add_library(ely::core ALIAS ely_core)

target_include_directories(ely_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_sources(ely_core
    PRIVATE
        src/chunk.cpp
        src/debug.cpp
)

ely_enable_warnings(ely_core)
