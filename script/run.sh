#!/bin/bash
script_path=$(realpath "$0")
script_dir=$(dirname "$script_path")
subject_dir=$(dirname "$script_dir")
echo $script_path $script_dir $subject_dir

cd $subject_dir

existing_index=$(ls ./result | grep -Eo 'output[0-9]+' | grep -Eo '[0-9]+' | sort -n)
index=0
for num in $existing_index; do
    if [ "$num" -ge "$index" ]; then
        index=$((num + 1))
    fi
done

echo $index

Jobs=24

clang-extract ../glibc-2.40-x86/test-skeleton.c -output=./result/x86-json$index.json -jobs=$Jobs >./result/x86-output$index 2>./result/x86-err$index &
pid=$!

clang-extract ../glibc-2.40-aarch64/test-skeleton.c -output=./result/aarch64-json$index.json -jobs=$Jobs >./result/aarch64-output$index 2>./result/aarch64-err$index