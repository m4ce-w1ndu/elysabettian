cmake_minimum_required(VERSION 3.10)
project(cxxelysabettian CXX)

set(exec ely)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)

if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /MTd /Z7")
    set(CMAKE_CXX_FLAGS_RELEASE "/Os")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -O0 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

include_directories(src/include)
include_directories(dep/fmt/include)
add_subdirectory(dep/fmt)

file(
    GLOB_RECURSE
    source_files 
    LIST_DIRECTORIES TRUE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp"
)

add_executable(${exec} ${header_files} ${source_files})
target_link_libraries(${exec} fmt)

foreach(source IN LISTS source_list)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()
