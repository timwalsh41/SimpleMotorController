#!/bin/bash

for arg in "$@"
do
    if [[ $arg == "--reset" ]]; then
        # Remove cached build directory - do this when changing
        # executables/content
        echo "Removing existing build directory"
        rm -rf build
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
echo
echo "Running tests"
cd build && ctest --output-on-failure
cd ..
