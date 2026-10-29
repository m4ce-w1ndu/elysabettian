include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)

FetchContent_MakeAvailable(Catch2)

add_executable(ely_tests
    src/test_main.cpp
    src/test_core.cpp
    src/test_scanner.cpp
    src/test_compiler.cpp
)

target_link_libraries(ely_tests
    PRIVATE
        Catch2::Catch2WithMain
        ely::core
        ely::compiler
)

ely_enable_warnings(ely_tests)

include(CTest)
include(Catch)
catch_discover_tests(ely_tests)
