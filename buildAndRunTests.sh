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
    echo
    echo "Running tests"
    echo "*******************"
    echo "NOTE THAT SOME TESTS ARE SENSITIVE TO MILLISECOND SCALE TIMING, THEY MAY FAIL DUE TO LATENCY IN THE HOST OS; CAN TRY TO RERUN TO VERIFY FAILURE"
    echo "*******************"
    echo
    echo
    cd build && ctest --output-on-failure
    cd ..
fi
