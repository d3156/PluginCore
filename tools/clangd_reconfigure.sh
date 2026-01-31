#!/bin/sh

start_dir=$PWD
output_file="$start_dir/compile_commands.json"

echo "[]" > "$output_file"  

find "$start_dir" -type f -name 'CMakeLists.txt' | while IFS= read -r cmake_file; do
    proj_dir=$(dirname "$cmake_file")
    echo "==== Generating compile_commands.json for: $proj_dir ===="

    (
        cd "$proj_dir" || exit 1
        # только генерация compile_commands.json без сборки
        cmake -S . -B build-debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
    )

    # объединяем, если файл существует
    compile_file="$proj_dir/build-debug/compile_commands.json"
    if [ -f "$compile_file" ]; then
        # объединяем массивы JSON безопасно, даже если файл пустой
        jq -s '.[0] + .[1]' "$output_file" "$compile_file" > "${output_file}.tmp" &&
        mv "${output_file}.tmp" "$output_file"
    fi
done

echo "compile_commands.json: $output_file"
