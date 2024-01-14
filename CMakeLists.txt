cmake_minimum_required(VERSION 3.10)
project(elysabettian)

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

find_package(fmt CONFIG REQUIRED)
include_directories(${FMT_INCLUDE_DIR})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/include")

file(
    GLOB_RECURSE
    source_files 
    LIST_DIRECTORIES TRUE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp"
)

add_executable(${CMAKE_PROJECT_NAME} ${header_files} ${source_files})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE fmt::fmt-header-only)

foreach(source IN LISTS source_list)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()
