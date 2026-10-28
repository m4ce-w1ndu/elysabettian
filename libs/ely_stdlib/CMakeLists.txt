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
)

target_sources(ely_stdlib
    PRIVATE
        # Add your .cpp files here (e.g. src/math.cpp, src/io.cpp)
)

ely_enable_warnings(ely_stdlib)
