cmake_minimum_required(VERSION 3.10)
project(CElysabettian)

set(CMAKE_C_STANDARD_REQUIRED 99)
set(CMAKE_C_STANDARD 11)

if (MSVC)
	set(CMAKE_C_FLAGS_DEBUG "/Od /Z7 /MTd TC")
	set(CMAKE_C_FLAGS_RELEASE "/Os /TC")
else()
	set(CMAKE_C_FLAGS_DEBUG "-g -Wall -fPIC -fsanitize=address -O0")
	set(CMAKE_C_FLAGS_RELEASE "-Os -fPIC")
endif()

file(GLOB_RECURSE SOURCES ./src "*.c" "*.h")

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})
