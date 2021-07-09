cmake_minimum_required(VERSION 3.10)
project(CElysabettian C)

if (MSVC)
    set(CMAKE_C_FLAGS_DEBUG "/Od /MTd /Z7 /TC")
    set(CMAKE_C_FLAGS_RELEASE "/Os /TC")
else()
    set(CMAKE_C_FLAGS_DEBUG "-Wall -O0 -g -fPIC -fsanitize=address")
    set(CMAKE_C_FLAGS_RELEASE "-Os -fPIC")
endif()

file(GLOB SOURCES "src/*.c" "src/*.h")

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})