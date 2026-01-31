#!/bin/sh

start_dir=$PWD

find "$start_dir" -type f -name 'CMakeLists.txt' | while IFS= read -r cmake_file; do
    proj_dir=$(dirname "$cmake_file")
    echo "==== Release build: $proj_dir ===="

    (
        cd "$proj_dir" || exit 1
        cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug &&
        cmake --build build-debug
        cp ./build-debug/compile_commands.json ${start_dir}/.compile_commands/compile_commands_$(basename "$proj_dir").json
    )
done
