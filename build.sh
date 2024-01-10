#!/bin/bash

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# check if conan is installed
if ! command_exists conan; then
    echo 'Error: the `conan` package manager is not installed. Cannot continue.'
    exit 1
fi

# check if cmake is installed
if ! command_exists cmake; then
    echo 'Error: `cmake` is not installed. Cannot continue.'
    exit 1
fi

# default mode will be release builds
mode='--release'
build_dir="build"
setup_build=false

if [ ! -d "$build_dir" ]; then
    setup_build=true
fi

if [ $# -gt 0 ]; then
    mode="$1"
fi

case "$mode" in
    --release | r)
        echo 'Starting release build...'

        if [ ! $setup_build ]; then
            cd "$build_dir"
            make
            exit 0
        fi

        conan install . --output-folder=build --build=missing
        cd "$build_dir"
        cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
        make
        exit 0

        ;;

    --debug | -d)
        echo 'Starting debug build...'
        if [ ! $setup_build ]; then
            cd "$build_dir"
            make
            exit 0
        fi

        conan install . --output-folder=build --build=missing
        cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
        cd "$build_dir"
        make
        exit 0

        ;;

    --clean | -c)
        echo 'Cleaning build...'
        rm -rf "$build_dir"
        rm -rf *.json
        ;;

    *)
        echo 'Error: invalid mode. Use --release (-r) or --debug (-d).'
        exit 1
        ;;
esac
