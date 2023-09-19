#!/bin/bash

run_tests=true

for arg in "$@"
do
    if [[ $arg == "--reset" ]]; then
        # Remove cached build directory - do this when changing
        # executables/content
        echo "Removing existing build directory"
        rm -rf build
    elif [[ $arg == "--norun" ]]; then
        run_tests=false
    else
        echo "unknown option: $arg"
    fi
done

clear
echo "Preparing build"
cmake -S . -B build
echo
echo "Building tests"
cmake --build build

if [[ "$run_tests" == true ]]; then
    echo
    echo "Running tests"
    cd build && ctest --output-on-failure
    cd ..
fi
