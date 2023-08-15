#!/bin/bash

for arg in "$@"
do
    if [[ $arg == "--refresh" ]]; then
        echo "Removing existing build directory"
        rm -rf build
    else
        echo "unknown option: $arg"
    fi
done

echo "Preparing build"
cmake -S . -B build
echo
echo "Building tests"
cmake --build build
echo
echo "Running tests"
cd build && ctest --output-on-failure
cd ..
