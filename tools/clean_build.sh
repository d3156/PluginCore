#!/bin/sh

start_dir="${1:-.}"

find "$start_dir" -type d -name build | while IFS= read -r d; do
    echo "Remove: $d"
    rm -rf -- "$d"
done

find "$start_dir" -type d -name build-debug | while IFS= read -r d; do
    echo "Remove: $d"
    rm -rf -- "$d"
done

find "$start_dir" -type d -name _CPack_Packages | while IFS= read -r d; do
    echo "Remove: $d"
    rm -rf -- "$d"
done
