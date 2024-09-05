#!/bin/bash

# Get a list of all the .cpp files.
filenames=$(find . -type f \( -name "*.cpp" -o -name ".h" -o -name ".inl" \))
echo $filenames

clang-format $filenames -i
